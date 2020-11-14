/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_channel_set.hpp"

#include "soc/rmt_reg.h"

namespace clk {

RMTChannelSet::RMTChannelSet(int qa_pin, int qb_pin)
    : ser_(RMTChannel(0, SER_GPIO, 0)),
      srclk_(RMTChannel(1, SRCLK_GPIO, 0)),
      rclk_(RMTChannel(2, RCLK_GPIO, 0)),
      r_(RMTChannel(3, OE_R_GPIO, 1)),
      g_(RMTChannel(4, OE_G_GPIO, 1)),
      b_(RMTChannel(5, OE_B_GPIO, 1)),
      qa_(RMTChannel(6, qa_pin, 0)),
      qb_(RMTChannel(7, qb_pin, 0)) {
}

void RMTChannelSet::Init() {
  ser_.Init();
  srclk_.Init();
  rclk_.Init();
  r_.Init();
  g_.Init();
  b_.Init();
  qa_.Init();
  qb_.Init();
}

void RMTChannelSet::Clear() {
  ser_.Clear();
  srclk_.Clear();
  rclk_.Clear();
  r_.Clear();
  g_.Clear();
  b_.Clear();
  qa_.Clear();
  qb_.Clear();
}

void RMTChannelSet::GenDigitA(uint8_t d, uint16_t rl, uint16_t gl,
                              uint16_t bl) {
  GenDigitDataSeq(d, 2);
  GenDigitControlSeq(&qa_, &qb_, rl, gl, bl);
}

void RMTChannelSet::GenDigitB(uint8_t d, uint16_t rl, uint16_t gl,
                              uint16_t bl) {
  GenDigitDataSeq(d, 2);
  GenDigitControlSeq(&qb_, &qa_, rl, gl, bl);
}

void RMTChannelSet::GenIdleSeq(uint16_t len) {
  rclk_.Off(len);
  srclk_.Off(len);
  ser_.Off(len);
  r_.Off(len);
  g_.Off(len);
  b_.Off(len);
  qa_.Off(len);
  qb_.Off(len);
}

IRAM void RMTChannelSet::Upload() {
  rclk_.Upload();
  srclk_.Upload();
  ser_.Upload();
  r_.Upload();
  g_.Upload();
  b_.Upload();
  qa_.Upload();
  qb_.Upload();
}

// This is especially time critical: we must kick off all channels as close to
// simultaneously as possible.
IRAM void RMTChannelSet::Start() {
  uint32_t sv1 = rclk_.conf1_start_;
  uint32_t sv2 = r_.conf1_start_;
  uint32_t rmt_reg_base = RMT_CH0CONF1_REG;
  __asm__ __volatile__(
      "rsil a8, 15\n"
      "s32i.n  %[sv1], %[rrb], 0*8\n"
      "s32i.n  %[sv1], %[rrb], 1*8\n"
      "s32i.n  %[sv1], %[rrb], 2*8\n"
      "s32i.n  %[sv2], %[rrb], 3*8\n"
      "s32i.n  %[sv2], %[rrb], 4*8\n"
      "s32i.n  %[sv2], %[rrb], 5*8\n"
      "s32i.n  %[sv1], %[rrb], 6*8\n"
      "s32i.n  %[sv1], %[rrb], 7*8\n"
      "wsr.ps  a8\n"
      : /* out */
      : /* in */[ rrb ] "a"(rmt_reg_base), [ sv1 ] "a"(sv1), [ sv2 ] "a"(sv2)
      : /* temp */ "a8", "memory");
}

IRAM void RMTChannelSet::AttachQ() {
  qa_.AttachPin();
  qb_.AttachPin();
}

IRAM void RMTChannelSet::DetachQ() {
  qa_.DetachPin();
  qb_.DetachPin();
}

void RMTChannelSet::GenDigitDataSeq(uint8_t digit, uint16_t len) {
  rclk_.Val(0, len * 16);
  for (uint8_t mask = 1; mask != 0; mask <<= 1) {
    // Set bit value in SER
    ser_.Val((digit & mask) != 0, len * 2);
    // Latch into shift register
    srclk_.Off(len);
    srclk_.On(len);
  }
  // Latch shift -> storage register.
  ser_.Off(len);
  srclk_.Off(len);
  rclk_.On(len * 2);
  ser_.OffTo(rclk_);
  srclk_.OffTo(rclk_);
  r_.OffTo(rclk_);
  g_.OffTo(rclk_);
  b_.OffTo(rclk_);
  qa_.OffTo(rclk_);
  qb_.OffTo(rclk_);
}

void RMTChannelSet::GenDigitControlSeq(RMTChannel *qa, RMTChannel *qb,
                                       uint16_t rl, uint16_t gl, uint16_t bl) {
  if (rl > 0) {  // 1.R Enable QA + R.
    r_.On(rl);
    qa->On(rl);
    // Idle channels
    rclk_.OffTo(r_);
    srclk_.OffTo(r_);
    ser_.OffTo(r_);
    // r_
    g_.OffTo(r_);
    b_.OffTo(r_);
    // qa
    qb->OffTo(r_);
  }
  if (gl > 0) {  // 1.G Disable R, enable G.
    r_.Off(gl);
    g_.On(gl);
    qa->On(gl);
    // Idle channels
    rclk_.OffTo(g_);
    srclk_.OffTo(g_);
    ser_.OffTo(g_);
    // r_
    // g_
    b_.OffTo(g_);
    // qa
    qb->OffTo(g_);
  }
  if (bl > 0) {  // 1.B Disable G, enable B.
    g_.Off(bl);
    b_.On(bl);
    qa->On(bl);
    // Idle channels
    rclk_.OffTo(b_);
    srclk_.OffTo(b_);
    ser_.OffTo(b_);
    r_.OffTo(b_);
    // g_
    // b_
    // qa
    qb->OffTo(b_);
  }
}

}  // namespace clk
