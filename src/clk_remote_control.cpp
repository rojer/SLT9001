/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_remote_control.hpp"

#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

#include "mgos.hpp"

#include "clk_rmt_input_channel.hpp"

namespace clk {

static constexpr uint16_t kBtnCodeHold = 0;              // "Button held down"
static constexpr int kBtnHoldMaxAgeMicros = 1000000;     // 1 second
static constexpr int kBtnReleaseTimeoutMicros = 200000;  // 200 ms
static constexpr int kTimerPeriodMs = 50;                // 100 ms

static RMTInputChannel s_ir_ch(7, IR_GPIO, false, 1, 50, 20000);
static RemoteControlButton s_last_btn = RemoteControlButton::kNone;
static int64_t s_last_btn_ts = 0;
static int64_t s_last_ev_ts = 0;
static int s_ev_count = 0;
typedef std::map<uint16_t, RemoteControlButton> ButtonMap;
static ButtonMap s_btn_map;

static void TimerCB();
static mgos::Timer s_timer_(TimerCB);

static bool ApproxEq(uint32_t val, uint32_t exp) {
  float vf = (float) val;
  float ef = (float) exp;
  return std::fabs(vf - ef) < 0.15f * exp;
}

// Parse 8-bit calibration sequence.
static bool ParseCalibSeq(const RMTChannel::Item *data, size_t *avg_lo,
                          size_t *avg_hi) {
  if (data[0].val != 0) return false;
  // All ticks should be about the same.
  uint32_t sum_lo = 0, sum_hi = 0;
  for (size_t i = 0; i <= 14; i += 2) {
    if (i <= 12) {
      LOG(LL_VERBOSE_DEBUG,
          ("%d | %d %d | %d %d", i, data[i].num_cycles, data[i + 2].num_cycles,
           data[i + 1].num_cycles, data[i + 3].num_cycles));
      if (!ApproxEq(data[i].num_cycles, data[i + 2].num_cycles)) return false;
      if (!ApproxEq(data[i + 1].num_cycles, data[i + 3].num_cycles))
        return false;
    }
    sum_lo += data[i].num_cycles;
    sum_hi += data[i + 1].num_cycles;
  }
  *avg_lo = sum_lo / 8;
  *avg_hi = sum_hi / 8;
  return true;
}

static mgos::StatusOr<uint16_t> DecodeSequence(const RMTChannel::Item *data,
                                               size_t len) {
  if (len == 2) {
    if (ApproxEq(data[0].num_cycles, 2200) &&
        ApproxEq(data[1].num_cycles, 550)) {
      return kBtnCodeHold;
    } else {
      return mgos::Errorf(STATUS_INVALID_ARGUMENT, "Invalid short sequence");
    }
  } else if (len != 66) {
    return mgos::Errorf(STATUS_INVALID_ARGUMENT, "Unexpected sequence length");
  }
  const auto *p = data;
  // First is a prologue: a positive pulse of about 4.5 ms
  if (p->val != 1 || !ApproxEq(p->num_cycles, 4500)) {
    return mgos::Errorf(STATUS_INVALID_ARGUMENT, "Invalid prologue");
  }
  p++;
  // Parse the "0" calibration sequence.
  uint32_t lo0 = 0, hi0 = 0;
  if (!ParseCalibSeq(p, &lo0, &hi0)) {
    return mgos::Errorf(STATUS_INVALID_ARGUMENT, "Invalid 0 cal seq");
  }
  p += 16;
  // Parse the "1" calibration sequence.
  uint32_t lo1 = 0, hi1 = 0;
  if (!ParseCalibSeq(p, &lo1, &hi1)) {
    return mgos::Errorf(STATUS_INVALID_ARGUMENT, "Invalid 1 cal seq");
  }
  p += 16;
  LOG(LL_VERBOSE_DEBUG, ("0: %d %d | 1: %d %d", lo0, hi0, lo1, hi1));
  // Low should be abot the same in both cases.
  if (!ApproxEq(lo0, lo1)) {
    return mgos::Errorf(STATUS_INVALID_ARGUMENT,
                        "Different 0 and 1 cal sequences");
  }
  // "1" must be different from "0".
  if (ApproxEq(hi0, hi1)) {
    return mgos::Errorf(STATUS_INVALID_ARGUMENT, "0 and 1 are the same");
  }
  p++;
  // Decode the sequence, extract key code.
  uint16_t code = 0, num_1 = 0;
  for (uint16_t i = 0, mask = 0x8000; i < 16; i++, mask >>= 1, p += 2) {
    LOG(LL_VERBOSE_DEBUG, ("%d | %d %d", i, p->val, p->num_cycles));
    if (ApproxEq(p->num_cycles, hi1)) {
      code |= mask;
      num_1++;
    } else if (!ApproxEq(p->num_cycles, hi0)) {
      return mgos::Errorf(STATUS_INVALID_ARGUMENT, "bad pulse at pos %d", i);
    }
  }
  if (num_1 != 8) {
    return mgos::Errorf(STATUS_INVALID_ARGUMENT, "expected 8 x 1s, got %d",
                        num_1);
  }
  return code;
}

static void TimerCB() {
  if (s_last_btn == RemoteControlButton::kNone) {
    s_timer_.Clear();
    return;
  }
  int64_t now = mgos_uptime_micros();
  if (now - s_last_btn_ts > kBtnReleaseTimeoutMicros) {
    RemoteControlButtonUpEventArg arg = {.btn = s_last_btn};
    mgos_event_trigger((int) RemoteControlButtonEvent::kButtonUp, &arg);
    s_timer_.Clear();
    s_last_btn = RemoteControlButton::kNone;
    return;
  }
  int64_t repeat_wait = std::max(1000000 / s_ev_count, 100000);
  if (now - s_last_ev_ts < repeat_wait) return;
  RemoteControlButtonDownEventArg arg = {.btn = s_last_btn, .repeat = true};
  mgos_event_trigger((int) RemoteControlButtonEvent::kButtonDown, &arg);
  s_last_ev_ts = now;
  s_ev_count++;
}

static void ProcessSequence() {
  auto st = DecodeSequence(s_ir_ch.data(), s_ir_ch.len());
  if (!st.ok()) {
    const auto ms = st.status().ToString();
    LOG(LL_DEBUG, ("Invalid control sequence: %s", ms.c_str()));
    // s_ir_ch.Dump();
    return;
  }
  uint16_t code = st.ValueOrDie();
  int64_t now = mgos_uptime_micros();
  RemoteControlButton btn = RemoteControlButton::kNone;
  if (code == kBtnCodeHold) {
    if (s_last_btn != RemoteControlButton::kNone &&
        (now - s_last_btn_ts < kBtnHoldMaxAgeMicros)) {
      btn = s_last_btn;
    } else {
      return;
    }
  } else {
    const auto &it = s_btn_map.find(code);
    if (it != s_btn_map.end()) {
      btn = it->second;
    }
  }
  if (btn != s_last_btn) {
    if (s_last_btn != RemoteControlButton::kNone) {
      RemoteControlButtonUpEventArg arg = {.btn = s_last_btn};
      mgos_event_trigger((int) RemoteControlButtonEvent::kButtonUp, &arg);
    }
    if (btn != RemoteControlButton::kNone) {
      RemoteControlButtonDownEventArg arg = {.btn = btn, .repeat = false};
      mgos_event_trigger((int) RemoteControlButtonEvent::kButtonDown, &arg);
      s_ev_count = 1;
      s_last_ev_ts = now;
      s_timer_.Reset(kTimerPeriodMs, MGOS_TIMER_REPEAT);
    }
    s_last_btn = btn;
  }
  if (btn != RemoteControlButton::kNone) {
    s_last_btn_ts = now;
  } else {
    s_timer_.Clear();
  }
}

static void IRRMTIntHandler2(void *arg) {
  s_ir_ch.Download();
  LOG(LL_VERBOSE_DEBUG, ("Downloaded %d items", (int) s_ir_ch.len()));
  ProcessSequence();
  s_ir_ch.Clear(true /* buf */, true /* mem */);
  mgos_gpio_enable_int(IR_GPIO);
  (void) arg;
}

IRAM static void IRRMTIntHandler(RMTChannel *ch, void *arg) {
  s_ir_ch.Stop();
  s_ir_ch.Detach();
  mgos_invoke_cb(IRRMTIntHandler2, nullptr, true /* from_isr */);
  (void) ch;
  (void) arg;
}

IRAM static void IRGPIOIntHandler(int pin, void *arg) {
  mgos_gpio_disable_int(IR_GPIO);
  s_ir_ch.Attach();
  s_ir_ch.ClearInt();
  s_ir_ch.EnableInt();
  s_ir_ch.Start();
  (void) pin;
  (void) arg;
}

static mgos::StatusOr<ButtonMap> ParseButtonMap(const char *map_spec) {
  ButtonMap btn_map;
  const char *p = map_spec;
  struct mg_str key, val;
  while ((p = mg_next_comma_list_entry(p, &key, &val)) != NULL) {
    std::string ks(key.p, key.len), vs(val.p, val.len);
    char *kend = nullptr, *vend = nullptr;
    int code = std::strtol(ks.c_str(), &kend, 0);
    int id = std::strtol(vs.c_str(), &vend, 0);
    if (*kend != '\0' || *vend != '\0' || code <= 0 || code > 0xffff ||
        id < 0 || id >= (int) RemoteControlButton::kMax) {
      return mgos::Errorf(STATUS_INVALID_ARGUMENT,
                          "Invalid button map entry %s=%s", ks.c_str(),
                          vs.c_str());
    }
    btn_map[code] = static_cast<RemoteControlButton>(id);
  }
  if (btn_map.empty()) {
    return mgos::Errorf(STATUS_INVALID_ARGUMENT, "Empty button map");
  }
  return btn_map;
}

void RemoteControlInit() {
  auto stv = ParseButtonMap(mgos_sys_config_get_clock_remote_button_map());
  if (stv.ok()) {
    s_btn_map = stv.ValueOrDie();
  } else {
    const auto &s = stv.status().ToString();
    LOG(LL_ERROR, ("Invalid button map: %s", s.c_str()));
  }
  s_ir_ch.Init();
  s_ir_ch.SetIntHandler(IRRMTIntHandler, nullptr);
  mgos_gpio_setup_input(IR_GPIO, MGOS_GPIO_PULL_NONE);
  mgos_gpio_set_int_handler_isr(IR_GPIO, MGOS_GPIO_INT_EDGE_NEG,
                                IRGPIOIntHandler, nullptr);
  mgos_gpio_enable_int(IR_GPIO);
}

}  // namespace clk
