/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_channel_set.hpp"

#include "mgos.h"

#include "soc/rmt_reg.h"

namespace clk {

RMTChannelSet::RMTChannelSet(int qa_pin, int qb_pin)
    : srclk_(RMTChannel(0, SRCLK_GPIO, 1, 0, false)),
      ser_(RMTChannel(1, SER_GPIO, 1, 0, false)),
      r_(RMTChannel(2, RCLK_R_GPIO, 1, 0, false)),
      g_(RMTChannel(3, RCLK_G_GPIO, 1, 0, false)),
      b_(RMTChannel(4, RCLK_B_GPIO, 1, 0, false)),
      qa_(RMTChannel(5, qa_pin, 1, 0, false)),
      qb_(RMTChannel(6, qb_pin, 1, 0, false)) {
}

void RMTChannelSet::Init() {
  srclk_.Init();
  ser_.Init();
  r_.Init();
  g_.Init();
  b_.Init();
  qa_.Init();
  qb_.Init();
}

void RMTChannelSet::Clear() {
  srclk_.Clear();
  ser_.Clear();
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
  srclk_.Off(len);
  ser_.Off(len);
  r_.Off(len);
  g_.Off(len);
  b_.Off(len);
  qa_.Off(len);
  qb_.Off(len);
}

IRAM void RMTChannelSet::Upload() {
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
  uint32_t sv = srclk_.conf1_start_;
  uint32_t rmt_reg_base = RMT_CH0CONF1_REG;
  __asm__ __volatile__(
      "rsil a8, 15\n"
      "s32i.n  %[sv], %[rrb], 0*8\n"
      "s32i.n  %[sv], %[rrb], 1*8\n"
      "s32i.n  %[sv], %[rrb], 2*8\n"
      "s32i.n  %[sv], %[rrb], 3*8\n"
      "s32i.n  %[sv], %[rrb], 4*8\n"
      "s32i.n  %[sv], %[rrb], 5*8\n"
      "s32i.n  %[sv], %[rrb], 6*8\n"
      "wsr.ps  a8\n"
      : /* out */
      : /* in */[ rrb ] "a"(rmt_reg_base), [ sv ] "a"(sv)
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
  r_.Dump();
  g_.Dump();
  b_.Dump();
  qa_.Dump();
  qb_.Dump();
  LOG(LL_INFO, ("%d+%d %d+%d | %d+%d %d+%d %d+%d | %d+%d %d+%d", srclk_.len_,
                srclk_.tot_len_, ser_.len_, ser_.tot_len_, r_.len_, r_.tot_len_,
                g_.len_, g_.tot_len_, b_.len_, b_.tot_len_, qa_.len_,
                qa_.tot_len_, qb_.len_, qb_.tot_len_));
}

void RMTChannelSet::GenDigitDataSeq(uint8_t digit, uint16_t len) {
  for (uint8_t mask = 1; mask != 0; mask <<= 1) {
    // Set bit value in SER
    ser_.Val((digit & mask) != 0, len * 2);
    // Latch into shift register
    srclk_.Off(len);
    srclk_.On(len);
  }
  r_.OffTo(srclk_);
  g_.OffTo(srclk_);
  b_.OffTo(srclk_);
  // Latch value to storage register.
  r_.On(len);
  g_.On(len);
  b_.On(len);
  // Prepare "off" value in shift register.
  for (int i = 0; i < 8; i++) {
    ser_.Val(1, len * 2);
    srclk_.Off(len);
    srclk_.On(len);
  }
  r_.OffTo(srclk_);
  g_.OffTo(srclk_);
  b_.OffTo(srclk_);
  // Idle channels
  qa_.OffTo(srclk_);
  qb_.OffTo(srclk_);
}

void RMTChannelSet::GenDigitControlSeq(RMTChannel *qa, RMTChannel *qb,
                                       uint16_t rl, uint16_t gl, uint16_t bl) {
  qa->Off(2);
  uint16_t max = 0;
  if (rl > 0) {
    r_.Off(rl + 2);
    if (rl > max) max = rl;
  }
  r_.On(1);
  if (gl > 0) {
    g_.Off(gl + 2);
    if (gl > max) max = gl;
  }
  g_.On(1);
  if (bl > 0) {
    b_.Off(bl + 2);
    if (bl > max) max = bl;
  }
  b_.On(1);
  qa->On(max + 1);
  qa->Off(1);
  r_.OffTo(*qa);
  g_.OffTo(*qa);
  b_.OffTo(*qa);
  // idle channels
  srclk_.OffTo(*qa);
  ser_.OffTo(*qa);
  qb->OffTo(*qa);
}

}  // namespace clk
