/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include <functional>

#include "mgos.hpp"
#include "mgos_app.h"
#include "mgos_bh1750.h"
#include "mgos_rpc.h"
#include "mgos_timers.hpp"

#include "clk_display_controller.hpp"

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

static char time_str[9] = {'1', '2', ':', '3', '4', ':', '5', '5'};
static uint16_t rl = 0, gl = 400, bl = 0, dl = 0;
static bool s_show_time = true;

static struct mgos_bh1750 *s_bh = NULL;

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
      (time_str[7] % 2 != 0 ? DisplayController::kDigitValueEmpty : 0b10101111);
  uint8_t digits[5] = {s_syms[time_str[0] - '0'], s_syms[time_str[1] - '0'], s2,
                       s_syms[time_str[3] - '0'], s_syms[time_str[4] - '0']};
  if (time_str[0] == '0') {
    digits[0] = DisplayController::kDigitValueEmpty;
  }
  SetDisplayDigits(digits, rl, gl, bl, dl);
  float lux = -1;
  if (s_bh != NULL) {
    lux = mgos_bh1750_read_lux(s_bh, nullptr);
  }
  LOG(LL_INFO, ("%s l %.2f", time_str, lux));
  (void) arg;
}

static mgos::ScopedTimer s_tmr(std::bind(TimerCB, nullptr));

bool InitApp() {
  mg_rpc_add_handler(mgos_rpc_get_global(), "Clock.SetColor",
                     "{s: %Q, r: %d, g: %d, b: %d, d: %d}", SetColorHandler,
                     nullptr);

  // Reset the light sensor.
  mgos_gpio_setup_output(DVI_GPIO, 0);
  mgos_msleep(1);
  mgos_gpio_write(DVI_GPIO, 1);

  uint8_t bh1750_addr = mgos_bh1750_detect();
  if (bh1750_addr != 0) {
    LOG(LL_INFO, ("Found BH1750 sensor at %#x", bh1750_addr));
    s_bh = mgos_bh1750_create(bh1750_addr);
    mgos_bh1750_set_config(s_bh, MGOS_BH1750_MODE_CONT_HIGH_RES_2,
                           mgos_sys_config_get_bh1750_mtime());
  }

  s_tmr.Reset(1000, MGOS_TIMER_REPEAT);
  return true;
}

}  // namespace clk

extern "C" {
enum mgos_app_init_result mgos_app_init(void) {
  return (clk::InitApp() ? MGOS_APP_INIT_SUCCESS : MGOS_APP_INIT_ERROR);
}
}
