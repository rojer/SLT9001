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
#include "mgos_veml7700.h"

#include "clk_br_curve.hpp"
#include "clk_display_controller.hpp"
#include "clk_remote_control.hpp"
#include "clk_rmt_input_channel.hpp"

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
static uint16_t s_rl = 0, s_gl = 0, s_bl = 0, s_dl = 0;
static uint16_t s_rl2 = 0, s_gl2 = 0, s_bl2 = 0;
static bool s_show_time = true;

static struct mgos_bh1750 *s_bh = NULL;
static struct mgos_veml7700 *s_veml = NULL;

static int CalcBrightness(float lux) {
  int br_pct = mgos_sys_config_get_clock_br();
  if (mgos_sys_config_get_clock_br_auto() && lux >= 0) {
    br_pct = (int) (lux * mgos_sys_config_get_clock_br_auto_f());
  }
  s_rl = (int) mgos_sys_config_get_clock_rl();
  s_gl = (int) mgos_sys_config_get_clock_gl();
  s_bl = (int) mgos_sys_config_get_clock_bl();
  if (br_pct < 0) {
    return br_pct;
  }
  if (br_pct < 1) br_pct = 1;
  if (br_pct > 100) br_pct = 100;
  BrightnessCurveEntry e = GetBrightnessCurveEntry(br_pct);
  s_rl *= e.sf;
  s_gl *= e.sf;
  s_bl *= e.sf;
  s_dl = e.dl;
  return br_pct;
}

static void UpdateDisplay() {
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
  float lux = -1;
  int br = mgos_sys_config_get_clock_br();
  if (mgos_sys_config_get_clock_br_auto()) {
    if (s_bh != NULL) {
      lux = mgos_bh1750_read_lux(s_bh, nullptr);
    } else if (s_veml != NULL) {
      lux = mgos_veml7700_read_lux(s_veml, true /* adjust */);
    }
  }
  br = CalcBrightness(lux);
  SetDisplayDigits(digits, s_rl, s_gl, s_bl, s_rl2, s_gl2, s_bl2, s_dl);
  LOG(LL_INFO, ("%s lux %.2f rl %d gl %d bl %d dl %d br %d", time_str, lux,
                s_rl, s_gl, s_bl, s_dl, br));
}

static mgos::Timer s_tmr(UpdateDisplay);

static void SetColorHandler(struct mg_rpc_request_info *ri, void *cb_arg,
                            struct mg_rpc_frame_info *fi, struct mg_str args) {
  char *s = NULL;
  int rl = -1, gl = -1, bl = -1, dl = -1, br = -2;
  int rl2 = -1, gl2 = -1, bl2 = -1;
  int8_t br_auto = -1;
  json_scanf(args.p, args.len, ri->args_fmt, &s, &rl, &gl, &bl, &rl2, &gl2, &bl2, &dl, &br,
             &br_auto);
  if (br != -2 && (br < -1 || br > 100)) {
    mg_rpc_send_errorf(ri, -1, "invalid %s", "br");
    return;
  }
  if (s != nullptr) {
    if (strlen(s) != 0) {
      s_show_time = false;
      strncpy(time_str, s, sizeof(time_str) - 1);
    } else {
      s_show_time = true;
    }
    free(s);
  }
  if (rl >= 0) {
    mgos_sys_config_set_clock_rl(rl);
  }
  if (gl >= 0) {
    mgos_sys_config_set_clock_gl(gl);
  }
  if (bl >= 0) {
    mgos_sys_config_set_clock_bl(bl);
  }
  if (dl >= 0) {
    s_dl = dl;
  }
  if (rl2 >= 0) {
    s_rl2 = rl2;
  }
  if (gl2 >= 0) {
    s_gl2 = gl2;
  }
  if (bl2 >= 0) {
    s_bl2 = bl2;
  }
  if (br != -2) {
    mgos_sys_config_set_clock_br(br);
  }
  if (br_auto != -1) {
    mgos_sys_config_set_clock_br_auto((br_auto != 0));
  }
  UpdateDisplay();
  mg_rpc_send_responsef(ri, nullptr);
  mgos_sys_config_save(&mgos_sys_config, false, nullptr);
}

static void RemoteButtonDownCB(int ev, void *ev_data, void *userdata) {
  const RemoteControlButtonDownEventArg *arg =
      (RemoteControlButtonDownEventArg *) ev_data;
  LOG(LL_INFO, ("BTN DOWN: %d", (int) arg->btn));
  (void) userdata;
  (void) ev;
}

static void RemoteButtonUpCB(int ev, void *ev_data, void *userdata) {
  const RemoteControlButtonUpEventArg *arg =
      (RemoteControlButtonUpEventArg *) ev_data;
  LOG(LL_INFO, ("BTN UP: %d", (int) arg->btn));
  (void) userdata;
  (void) ev;
}

static void ButtonCB(int pin, void *arg) {
  switch (pin) {
    case K1_GPIO:
      LOG(LL_INFO, ("K1 (SET)"));
      break;
    case K2_GPIO:
      LOG(LL_INFO, ("K2 (UP)"));
      break;
    case K3_GPIO:
      LOG(LL_INFO, ("K3 (DOWN)"));
      break;
  }
  (void) arg;
}

static void PeekHandler(struct mg_rpc_request_info *ri, void *cb_arg,
                        struct mg_rpc_frame_info *fi, struct mg_str args) {
  uint32_t addr = 0;
  json_scanf(args.p, args.len, ri->args_fmt, &addr);
  if (addr == 0) {
    mg_rpc_send_errorf(ri, -1, "%s is required", "addr");
    return;
  }
  uint32_t val = *((uint32_t *) addr);
  mg_rpc_send_responsef(ri, "{val: %u}", val);
  (void) fi;
  (void) cb_arg;
}

static void PokeHandler(struct mg_rpc_request_info *ri, void *cb_arg,
                        struct mg_rpc_frame_info *fi, struct mg_str args) {
  uint32_t addr = 0, val = 0;
  json_scanf(args.p, args.len, ri->args_fmt, &addr, &val);
  if (addr == 0) {
    mg_rpc_send_errorf(ri, -1, "%s is required", "addr");
    return;
  }
  *((uint32_t *) addr) = val;
  mg_rpc_send_responsef(ri, nullptr);
  (void) fi;
  (void) cb_arg;
}

bool InitApp() {
  mg_rpc_add_handler(mgos_rpc_get_global(), "Clock.SetColor",
                     "{s: %Q, r: %d, g: %d, b: %d, r2: %d, g2: %d, b2: %d, d: %d, "
                     "br: %d, br_auto: %B}",
                     SetColorHandler, nullptr);
  mg_rpc_add_handler(mgos_rpc_get_global(), "Clock.Peek", "{addr: %u}",
                     PeekHandler, nullptr);
  mg_rpc_add_handler(mgos_rpc_get_global(), "Clock.Poke", "{addr: %u, val: %u}",
                     PokeHandler, nullptr);

#ifdef DVI_GPIO
  // Reset the light sensor.
  mgos_gpio_setup_output(DVI_GPIO, 0);
  mgos_msleep(1);
  mgos_gpio_write(DVI_GPIO, 1);
#endif

  mgos_gpio_set_mode(BUZZ_GPIO, MGOS_GPIO_MODE_OUTPUT_OD);
  mgos_gpio_write(BUZZ_GPIO, 1);

  struct mgos_i2c *i2c_bus = mgos_i2c_get_bus(0);
  uint8_t bh1750_addr = mgos_bh1750_detect_i2c(i2c_bus);
  if (bh1750_addr != 0) {
    LOG(LL_INFO, ("Found BH1750 sensor at %#x", bh1750_addr));
    s_bh = mgos_bh1750_create(bh1750_addr);
    mgos_bh1750_set_config(s_bh, MGOS_BH1750_MODE_CONT_HIGH_RES_2,
                           mgos_sys_config_get_clock_bh1750_mtime());
  } else if (mgos_veml7700_detect(i2c_bus)) {
    LOG(LL_INFO, ("Found VEML7700 sensor"));
    s_veml = mgos_veml7700_create(i2c_bus);
    mgos_veml7700_set_cfg(
        s_veml, MGOS_VEML7700_CFG_ALS_IT_100 | MGOS_VEML7700_CFG_ALS_GAIN_1,
        MGOS_VEML7700_PSM_0);
  } else {
    LOG(LL_ERROR, ("No light sensor found!"));
  }

  RemoteControlInit();
  mgos_event_add_handler((int) RemoteControlButtonEvent::kButtonDown,
                         RemoteButtonDownCB, nullptr);
  mgos_event_add_handler((int) RemoteControlButtonEvent::kButtonUp,
                         RemoteButtonUpCB, nullptr);

  mgos_gpio_set_button_handler(K1_GPIO, MGOS_GPIO_PULL_UP,
                               MGOS_GPIO_INT_EDGE_NEG, 20, ButtonCB, nullptr);
  mgos_gpio_set_button_handler(K2_GPIO, MGOS_GPIO_PULL_UP,
                               MGOS_GPIO_INT_EDGE_NEG, 20, ButtonCB, nullptr);
  mgos_gpio_set_button_handler(K3_GPIO, MGOS_GPIO_PULL_UP,
                               MGOS_GPIO_INT_EDGE_NEG, 20, ButtonCB, nullptr);

  s_tmr.Reset(1000, MGOS_TIMER_REPEAT);

  s_rl2 = mgos_sys_config_get_clock_rl();
  s_gl2 = mgos_sys_config_get_clock_gl();
  s_bl2 = mgos_sys_config_get_clock_bl();

  return true;
}

}  // namespace clk

extern "C" {
enum mgos_app_init_result mgos_app_init(void) {
  return (clk::InitApp() ? MGOS_APP_INIT_SUCCESS : MGOS_APP_INIT_ERROR);
}
}
