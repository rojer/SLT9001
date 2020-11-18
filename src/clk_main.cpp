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
      // Detach from RMT for now.
      banks[i].Init();
      banks[i].DetachQ();
    }
  }
};

// Two identical banks of channel sets are maintained:
// the active one is used for display, when a change is needed it is made to the
// inactive set and swapped out.
static RMTChannelSetBank s_sets[2] = {
    {.banks = {RMTChannelSet(Q1_GPIO, Q2_GPIO), RMTChannelSet(Q5_GPIO, -1),
               RMTChannelSet(Q3_GPIO, Q4_GPIO)},
     .current = 0},
    {.banks = {RMTChannelSet(Q1_GPIO, Q2_GPIO), RMTChannelSet(Q5_GPIO, -1),
               RMTChannelSet(Q3_GPIO, Q4_GPIO)},
     .current = 0},
};
static bool s_switch_set = false;
static int s_active_set = 0;

static char time_str[9] = {'1', '1', ':', '1', '1', ':', '1', '1'};
static uint16_t rl = 8, gl = 16, bl = 24, dl = 100;

// Advance to the next bank in the active set.
IRAM void RMTIntHandler(void *arg) {
  mgos_gpio_toggle(16);
  RMT.int_clr.val = RMT_CH0_TX_END_INT_CLR;
  RMTChannelSetBank *abs = &s_sets[s_active_set];
  RMTChannelSet *old_bank = &abs->banks[abs->current];
  abs->current++;
  if (abs->current == ARRAY_SIZE(abs->banks)) {
    if (s_switch_set) {
      s_active_set ^= 1;
      abs = &s_sets[s_active_set];
      abs->current = 0;
      s_switch_set = false;
    } else {
      abs->current = 0;
    }
  }
  old_bank->DetachQ();
  RMTChannelSet *new_bank = &abs->banks[abs->current];
  new_bank->Upload();
  new_bank->Start();
  new_bank->AttachQ();
  mgos_gpio_toggle(16);
  (void) arg;
}

void SendDigits(uint8_t digits[5]) {
  static bool s_started = false;

  s_switch_set = false;
  int inactive_set = (s_active_set ^ 1);
  RMTChannelSetBank *ibs = &s_sets[inactive_set];
  RMTChannelSet *ib0 = &ibs->banks[0];
  ib0->Clear();
  ib0->GenDigitA(digits[0], rl, gl, bl);
  ib0->GenDigitB(digits[1], rl, gl, bl);
  RMTChannelSet *ib1 = &ibs->banks[1];
  ib1->Clear();
  ib1->GenDigitA(digits[2], rl, gl, bl);
  RMTChannelSet *ib2 = &ibs->banks[2];
  ib2->Clear();
  ib2->GenDigitA(digits[3], rl, gl, bl);
  ib2->GenDigitB(digits[4], rl, gl, bl);

  if (dl > 0) {
    ib2->GenIdleSeq(dl);
  }

  ib0->Dump();

  if (!s_started) {
    RMTChannelSet *ab0 = &s_sets[inactive_set].banks[0];
    ab0->Upload();
    s_active_set = inactive_set;
    RMT.int_clr.val = RMT_CH0_TX_END_INT_CLR;
    RMT.int_ena.val = RMT_CH0_TX_END_INT_CLR;
    ab0->Start();
    ab0->AttachQ();
    s_started = true;
  } else {
    s_switch_set = true;
  }
}

static void SetColorHandler(struct mg_rpc_request_info *ri, void *cb_arg,
                            struct mg_rpc_frame_info *fi, struct mg_str args) {
  char *s = NULL;
  int r = -1, g = -1, b = -1, d = -1;
  json_scanf(args.p, args.len, ri->args_fmt, &s, &r, &g, &b, &d);
  if (s != nullptr) {
    strncpy(time_str, s, sizeof(time_str) - 1);
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
  // mgos_strftime(time_str, sizeof(time_str), "%H:%M:%S", (int) mg_time());
  uint8_t s2 = (time_str[7] % 2 == 0 ? 0xff : 0b10101111);
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
