/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_channel_set.hpp"

#include "mgos.h"

#include "soc/rmt_reg.h"

namespace clk {

RMTChannelSet::RMTChannelSet(int qa_pin, int qb_pin)
    : ser_(RMTChannel(0, SER_GPIO, 1, 0, false)),
      srclk_(RMTChannel(1, SRCLK_GPIO, 1, 0, false)),
      rclk_(RMTChannel(2, RCLK_GPIO, 1, 0, false)),
      r_(RMTChannel(3, OE_R_GPIO, 0, 1, false)),
      g_(RMTChannel(4, OE_G_GPIO, 0, 1, false)),
      b_(RMTChannel(5, OE_B_GPIO, 0, 1, false)),
      qa_(RMTChannel(6, qa_pin, 1, 0, false)),
      qb_(RMTChannel(7, qb_pin, 1, 0, false)) {
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
  GenDigitDataSeq(d, 1);
  GenDigitControlSeq(&qa_, &qb_, rl, gl, bl);
}

void RMTChannelSet::GenDigitB(uint8_t d, uint16_t rl, uint16_t gl,
                              uint16_t bl) {
  GenDigitDataSeq(d, 1);
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

void RMTChannelSet::Dump() {
  srclk_.Dump();
  ser_.Dump();
  rclk_.Dump();
  r_.Dump();
  g_.Dump();
  b_.Dump();
  qa_.Dump();
  qb_.Dump();
  LOG(LL_INFO,
      ("%d+%d %d+%d %d+%d | %d+%d %d+%d %d+%d | %d+%d %d+%d", srclk_.len_,
       srclk_.tot_len_, ser_.len_, ser_.tot_len_, rclk_.len_, rclk_.tot_len_,
       r_.len_, r_.tot_len_, g_.len_, g_.tot_len_, b_.len_, b_.tot_len_,
       qa_.len_, qa_.tot_len_, qb_.len_, qb_.tot_len_));
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
  rclk_.On(len);
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
  uint16_t max = 0;
  if (rl > 0) {
    r_.On(rl);
    if (rl > max) max = rl;
  }
  if (gl > 0) {
    g_.On(gl);
    if (gl > max) max = gl;
  }
  if (bl > 0) {
    b_.On(bl);
    if (bl > max) max = bl;
  }
  qa->On(max);
  r_.OffTo(*qa);
  g_.OffTo(*qa);
  b_.OffTo(*qa);
  // idle channels
  rclk_.Off(max);
  srclk_.Off(max);
  ser_.Off(max);
  qb->OffTo(*qa);
}

}  // namespace clk
