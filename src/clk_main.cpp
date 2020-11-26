/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include <functional>

#include "mgos.hpp"
#include "mgos_app.h"
#include "mgos_rpc.h"
#include "mgos_timers.hpp"

#include "driver/periph_ctrl.h"
#include "rom/ets_sys.h"
#include "soc/rmt_reg.h"
#include "soc/rmt_struct.h"

#include "clk_rmt_channel_set.hpp"

namespace clk {

static const uint8_t s_syms[16] = {
    0x03,  // 0000 0011, "0"
    0x9f,  // 1001 1111, "1"
    0x25,  // 0010 0101, "2"
    0x0d,  // 0000 1101, "3"
    0x99,  // 1001 1001, "4"
    0x49,  // 0100 1001, "5"
    0x41,  // 0100 0001, "6"
    0x1f,  // 0001 1111, "7"
    0x01,  // 0000 0001, "8"
    0x09,  // 0000 1001, "9"
    0x11,  // 0001 0001, "A"
    0xc1,  // 1100 0001, "b"
    0x63,  // 0110 0011, "C"
    0x85,  // 1000 0101, "d"
    0x61,  // 0110 0001, "E"
    0x71,  // 0111 0001, "F"
};

struct RMTChannelSetBank {
  RMTChannelSet banks[3];
  int current;

  void Init() {
    for (int i = 0; i < 3; i++) {
      banks[i].Init();
    }
  }
};

// Two identical banks of channel sets are maintained:
// the active one is used for display, when a change is needed it is made to the
// inactive set and swapped out.
static RMTChannelSet s_sets[2] = {RMTChannelSet(), RMTChannelSet()};
static bool s_switch_set = false;
static int s_active_set = 0;

static char time_str[9] = {'2', '5', ':', '1', '2', ':', '1', '1'};
static uint16_t rl = 1000, gl = 1000, bl = 0, dl = 1000;
static bool s_show_time = true;

// Advance to the next bank in the active set.
IRAM void RMTIntHandler(void *arg) {
  mgos_gpio_toggle(16);
  RMT.int_clr.val = RMT_CH0_TX_END_INT_CLR;
  RMTChannelSet *s = &s_sets[s_active_set];
  if (s_switch_set) {
    s_active_set ^= 1;
    s = &s_sets[s_active_set];
    s_switch_set = false;
  }
  s->Upload();
  s->Start();
  mgos_gpio_toggle(16);
  (void) arg;
}

void SendDigits(uint8_t digits[5]) {
  static bool s_started = false;

  s_switch_set = false;
  int inactive_set = (s_active_set ^ 1);
  RMTChannelSet *s = &s_sets[inactive_set];
  s->Clear();
  s->GenDigitSeq(1, digits[0], 1, rl, gl, bl, dl);
  s->GenDigitSeq(2, digits[1], 1, rl, gl, bl, dl);
  s->GenDigitSeq(5, digits[2], 1, rl, gl, bl, dl);
  s->GenDigitSeq(3, digits[3], 1, rl, gl, bl, dl);
  s->GenDigitSeq(4, digits[4], 1, rl, gl, bl, dl);

  // s->Dump();

  if (!s_started) {
    s_active_set = inactive_set;
    s->Upload();
    s->Start();
    s_started = true;
  } else {
    s_switch_set = true;
  }
  RMT.int_ena.val = RMT_CH0_TX_END_INT_ENA;
}

static void SetColorHandler(struct mg_rpc_request_info *ri, void *cb_arg,
                            struct mg_rpc_frame_info *fi, struct mg_str args) {
  char *s = NULL;
  int r = -1, g = -1, b = -1, d = -1;
  json_scanf(args.p, args.len, ri->args_fmt, &s, &r, &g, &b, &d);
  if (s != nullptr) {
    if (strlen(s) != 0) {
      s_show_time = false;
      strncpy(time_str, s, sizeof(time_str) - 1);
    } else {
      s_show_time = true;
    }
    free(s);
  }
  if (r >= 0) {
    rl = r;
  }
  if (g >= 0) {
    gl = g;
  }
  if (b >= 0) {
    bl = b;
  }
  if (d >= 0) {
    dl = d;
  }
  mg_rpc_send_responsef(ri, nullptr);
}

static void TimerCB(void *arg) {
  if (s_show_time) {
    mgos_strftime(time_str, sizeof(time_str), "%H:%M:%S", (int) mg_time());
  }
  uint8_t s2 =
      (time_str[7] % 2 == 0 ? RMTChannelSet::kDigitValueEmpty : 0b10101111);
  uint8_t digits[5] = {s_syms[time_str[0] - '0'], s_syms[time_str[1] - '0'], s2,
                       s_syms[time_str[3] - '0'], s_syms[time_str[4] - '0']};
  SendDigits(digits);
  LOG(LL_INFO, ("%s", time_str));
  (void) arg;
}

static mgos::ScopedTimer s_tmr(std::bind(TimerCB, nullptr));

bool InitApp() {
  periph_module_enable(PERIPH_RMT_MODULE);
  RMT.apb_conf.fifo_mask = 1;
  RMT.apb_conf.mem_tx_wrap_en = 0;
  for (int i = 0; i < 2; i++) {
    s_sets[i].Init();
  }

  mgos_gpio_setup_output(16, 0);

  intr_handle_t inth;
  esp_intr_alloc(ETS_RMT_INTR_SOURCE, 0, RMTIntHandler, nullptr, &inth);
  esp_intr_set_in_iram(inth, true);

  mg_rpc_add_handler(mgos_rpc_get_global(), "Clock.SetColor",
                     "{s: %Q, r: %d, g: %d, b: %d, d: %d}", SetColorHandler,
                     nullptr);

  s_tmr.Reset(1000, MGOS_TIMER_REPEAT);
  return true;
}

}  // namespace clk

extern "C" {
enum mgos_app_init_result mgos_app_init(void) {
  return (clk::InitApp() ? MGOS_APP_INIT_SUCCESS : MGOS_APP_INIT_ERROR);
}
}
