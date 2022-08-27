/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_display_controller.hpp"

#include "mgos.h"

#include "soc/rmt_reg.h"

namespace clk {

DisplayController::DisplayController(void(int_handler)())
    : srclk_(RMTOutputChannel(0, SRCLK_GPIO, 1, 0, false /* loop */)),
      ser_(RMTOutputChannel(1, SER_GPIO, 0, 1, false /* loop */)),
      qser_(RMTOutputChannel(2, QSER_GPIO, 0, 1, false /* loop */)),
      rclk_(RMTOutputChannel(3, RCLK_GPIO, 1, 0, false /* loop */)),
      r_(RMTOutputChannel(4, OE_R_GPIO, 0, 1, false /* loop */)),
      g_(RMTOutputChannel(5, OE_G_GPIO, 0, 1, false /* loop */)),
      b_(RMTOutputChannel(6, OE_B_GPIO, 0, 1, false /* loop */)),
      int_handler_(int_handler) {
}

void DisplayController::Init() {
  srclk_.Init();
  ser_.Init();
  qser_.Init();
  rclk_.Init();
  r_.Init();
  g_.Init();
  b_.Init();
}

IRAM void DisplayController::Attach() {
  srclk_.SetIntHandler(ChannelIntHandler, this);
  srclk_.Attach();
  ser_.Attach();
  qser_.Attach();
  rclk_.Attach();
  r_.Attach();
  g_.Attach();
  b_.Attach();
}

IRAM void DisplayController::Detach() {
  srclk_.DisableInt();
  srclk_.SetIntHandler(nullptr, nullptr);
  srclk_.Detach();
  ser_.Detach();
  qser_.Detach();
  rclk_.Detach();
  r_.Detach();
  g_.Detach();
  b_.Detach();
}

void DisplayController::Clear() {
  srclk_.Clear();
  ser_.Clear();
  qser_.Clear();
  rclk_.Clear();
  r_.Clear();
  g_.Clear();
  b_.Clear();
}

void DisplayController::GenDigitSeq(uint8_t qn, uint8_t d, uint16_t len,
                                    uint16_t rl, uint16_t gl, uint16_t bl,
                                    uint16_t dl) {
  // Set up shift registers
  int qbn = qn + 2;
  // 5 - 7, 4 - 6, 3 - 5, 2 - 4, 1 - 3,
  for (int i = 1; i < 8; i++) {
    // Set bit value in SER
    ser_.Set((d & (1 << i)) == 0, len * 2);
    // Set Q value in the Q shift register
    qser_.Set((i == qbn && d != kDigitValueEmpty), len * 2);
    // Latch into shift register
    srclk_.Off(len);
    srclk_.On(len);
  }
  rclk_.OffTo(srclk_);
  r_.OffTo(srclk_);
  g_.OffTo(srclk_);
  b_.OffTo(srclk_);
  // Latch, activate Q.
  rclk_.On(len);
  // Shift in the sequence to turn off Q.
  for (int i = 0; i < 5; i++) {
    ser_.Off(len * 2);
    qser_.Off(len * 2);
    srclk_.Off(len);
    srclk_.On(len);
  }
  // Activate segments.
  uint16_t max = 0;
  r_.On(rl);
  if (rl > max) max = rl;
  g_.On(gl);
  if (gl > max) max = gl;
  b_.On(bl);
  if (bl > max) max = bl;
  rclk_.Off(max);
  if (max < 5 * 2) {
    rclk_.Off(5 * 2 - max);
  }
  // Pull everything up to max.
  r_.OffTo(rclk_);
  g_.OffTo(rclk_);
  b_.OffTo(rclk_);
  // Turn off Q.
  rclk_.On(len);
  // Pull everything up.
  r_.OffTo(rclk_);
  g_.OffTo(rclk_);
  b_.OffTo(rclk_);
  srclk_.OffTo(rclk_);
  ser_.OffTo(rclk_);
  qser_.OffTo(rclk_);
  // Idle sequence.
  GenIdleSeq(dl);
}

void DisplayController::GenIdleSeq(uint16_t dl) {
  if (dl == 0) return;
  srclk_.Off(dl);
  ser_.Off(dl);
  qser_.Off(dl);
  rclk_.Off(dl);
  r_.Off(dl);
  g_.Off(dl);
  b_.Off(dl);
}

IRAM void DisplayController::Upload() {
  srclk_.Upload();
  ser_.Upload();
  qser_.Upload();
  rclk_.Upload();
  r_.Upload();
  g_.Upload();
  b_.Upload();
}

// This is especially time critical: we must kick off all channels as close to
// simultaneously as possible.
IRAM void DisplayController::Start() {
  uint32_t sv1 = srclk_.conf1_start_;
  uint32_t sv2 = r_.conf1_start_;
  uint32_t rmt_reg_base = RMT_CH0CONF1_REG;
  srclk_.ClearInt();
  __asm__ __volatile__(
      "rsil a8, 15\n"
      "s32i.n  %[sv1], %[rrb], 0*8\n"
      "s32i.n  %[sv2], %[rrb], 1*8\n"
      "s32i.n  %[sv2], %[rrb], 2*8\n"
      "s32i.n  %[sv1], %[rrb], 3*8\n"
      "s32i.n  %[sv2], %[rrb], 4*8\n"
      "s32i.n  %[sv2], %[rrb], 5*8\n"
      "s32i.n  %[sv2], %[rrb], 6*8\n"
      "wsr.ps  a8\n"
      : /* out */
      : /* in */[ rrb ] "a"(rmt_reg_base), [ sv1 ] "a"(sv1), [ sv2 ] "a"(sv2)
      : /* temp */ "a8", "memory");
  srclk_.EnableInt();
}

void DisplayController::Dump() {
  srclk_.Dump();
  ser_.Dump();
  qser_.Dump();
  rclk_.Dump();
  r_.Dump();
  g_.Dump();
  b_.Dump();
  LOG(LL_INFO, ("%d+%d %d+%d %d+%d %d+%d | %d+%d %d+%d %d+%d", srclk_.len_,
                srclk_.tot_len_, ser_.len_, ser_.tot_len_, qser_.len_,
                qser_.tot_len_, rclk_.len_, rclk_.tot_len_, r_.len_,
                r_.tot_len_, g_.len_, g_.tot_len_, b_.len_, b_.tot_len_));
}

void DisplayController::SetDigits(uint8_t digits[5],
    uint16_t rl, uint16_t gl, uint16_t bl,
    uint16_t rl2, uint16_t gl2, uint16_t bl2,
    uint16_t dl) {
  Clear();
#if QMAP == 1
  GenDigitSeq(1, digits[0], 1, rl, gl, bl, dl);
  GenDigitSeq(2, digits[1], 1, rl, gl, bl, dl);
  GenDigitSeq(5, digits[2], 1, rl2, gl2, bl2, dl);
  GenDigitSeq(3, digits[3], 1, rl, gl, bl, dl);
  GenDigitSeq(4, digits[4], 1, rl, gl, bl, dl);
#elif QMAP == 2
  GenDigitSeq(4, digits[0], 1, rl, gl, bl, dl);
  GenDigitSeq(3, digits[1], 1, rl, gl, bl, dl);
  GenDigitSeq(0, digits[2], 1, rl2, gl2, bl2, dl);
  GenDigitSeq(2, digits[3], 1, rl, gl, bl, dl);
  GenDigitSeq(1, digits[4], 1, rl, gl, bl, dl);
#else
#error "Q mapping not set"
#endif
}

// static
IRAM void DisplayController::ChannelIntHandler(RMTChannel *ch, void *arg) {
  DisplayController *ctl = static_cast<DisplayController *>(arg);
  ctl->int_handler_();
}

// Two controllers: one is active, the other is inactive and can be updated.
extern DisplayController s_ctls[2];
bool s_started = false;
static bool s_switch_ctl = false;
static int s_active_ctl = 0;

IRAM void DisplayIntHandler() {
#ifdef DISPLAY_DEBUG_GPIO
  mgos_gpio_toggle(DISPLAY_DEBUG_GPIO);
#endif
  RMT.int_clr.ch0_tx_end = true;
  DisplayController *ctl = &s_ctls[s_active_ctl];
  if (s_switch_ctl) {
    ctl->Detach();
    s_active_ctl ^= 1;
    ctl = &s_ctls[s_active_ctl];
    ctl->Attach();
    s_switch_ctl = false;
  }
  ctl->Upload();
  ctl->Start();
#ifdef DISPLAY_DEBUG_GPIO
  mgos_gpio_toggle(DISPLAY_DEBUG_GPIO);
#endif
}

DisplayController s_ctls[2] = {DisplayController(DisplayIntHandler),
                               DisplayController(DisplayIntHandler)};

void SetDisplayDigits(uint8_t digits[5],
    uint16_t rl, uint16_t gl, uint16_t bl,
    uint16_t rl2, uint16_t gl2, uint16_t bl2,
                      uint16_t dl) {
  s_switch_ctl = false;
  if (!s_started) {
    s_ctls[0].Init();
    s_ctls[1].Init();
#ifdef DISPLAY_DEBUG_GPIO
    mgos_gpio_setup_output(DISPLAY_DEBUG_GPIO, 0);
#endif
  }
  int inactive_ctl = (s_active_ctl ^ 1);
  DisplayController *ctl = &s_ctls[inactive_ctl];
  ctl->SetDigits(digits, rl, gl, bl, rl2, gl2, bl2, dl);

  if (!s_started) {
    s_active_ctl = inactive_ctl;
    ctl->Upload();
    ctl->Attach();
    ctl->Start();
    s_started = true;
  } else {
    s_switch_ctl = true;
  }
}

}  // namespace clk
