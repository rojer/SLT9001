/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_channel_set.hpp"

#include "mgos.h"

#include "soc/rmt_reg.h"

namespace clk {

RMTChannelSet::RMTChannelSet()
    : srclk_(RMTChannel(0, SRCLK_GPIO, 1, 0, false)),
      ser_(RMTChannel(1, SER_GPIO, 1, 0, false)),
      qser_(RMTChannel(2, QSER_GPIO, 1, 0, false)),
      r_(RMTChannel(3, RCLK_R_GPIO, 1, 0, false)),
      g_(RMTChannel(4, RCLK_G_GPIO, 1, 0, false)),
      b_(RMTChannel(5, RCLK_B_GPIO, 1, 0, false)),
      q_(RMTChannel(6, Q_GPIO, 1, 0, false)) {
}

void RMTChannelSet::Init() {
  srclk_.Init();
  ser_.Init();
  qser_.Init();
  r_.Init();
  g_.Init();
  b_.Init();
  q_.Init();
}

void RMTChannelSet::Clear() {
  srclk_.Clear();
  ser_.Clear();
  qser_.Clear();
  r_.Clear();
  g_.Clear();
  b_.Clear();
  q_.Clear();
}

void RMTChannelSet::GenDigit(uint8_t qn, uint8_t d, uint16_t len, uint16_t rl,
                             uint16_t gl, uint16_t bl) {
  int qbn = qn + 2;
  for (int i = 1; i < 8; i++) {
    // Set bit value in SER
    ser_.Val((d & (1 << i)) != 0, len * 2);
    qser_.Val(!(i == qbn), len * 2);
    // Latch into shift register
    srclk_.Off(len);
    srclk_.On(len);
  }
  uint16_t drain_q = 5;
  uint16_t min_d = 7 * 2 * len + drain_q;
  // Latch value to storage register.
  r_.OffTo(srclk_);
  r_.Val((rl >= min_d), len);
  g_.OffTo(srclk_);
  g_.Val((gl >= min_d), len);
  b_.OffTo(srclk_);
  b_.Val((bl >= min_d), len);
  q_.OffTo(srclk_);
  q_.Val((d != kDigitValueEmpty), len);
  // Prepare "off" value in shift register.
  for (int i = 0; i < 7; i++) {
    ser_.On(len * 2);
    qser_.On(len * 2);
    srclk_.Off(len);
    srclk_.On(len);
  }
  r_.Off(min_d - 1);
  g_.Off(min_d - 1);
  b_.Off(min_d - 1);

  uint16_t max = 0;
  RMTChannel *max_ch = nullptr;
  if (rl > min_d) {
    r_.Off(rl - min_d);
    if (rl > max) {
      max = rl;
      max_ch = &r_;
    }
  }
  r_.On(len);

  if (gl > min_d) {
    g_.Off(gl - min_d);
    if (gl > max) {
      max = gl;
      max_ch = &g_;
    }
  }
  g_.On(len);

  if (bl > min_d) {
    b_.Off(bl - min_d);
    if (bl > max) {
      max = bl;
      max_ch = &b_;
    }
  }
  b_.On(len);

  if (max > 0) {
    r_.OffTo(*max_ch);
    g_.OffTo(*max_ch);
    b_.OffTo(*max_ch);
    q_.Off(max - drain_q - 1);
    q_.On(len);
    q_.OffTo(*max_ch);
  } else {
    q_.Off(min_d - 1);
    q_.On(len);
    r_.OffTo(q_);
    g_.OffTo(q_);
    b_.OffTo(q_);
  }

  // Idle channels.
  srclk_.OffTo(q_);
  ser_.OffTo(q_);
  qser_.OffTo(q_);
}

void RMTChannelSet::GenIdleSeq(uint16_t len) {
  if (len == 0) return;
  srclk_.Off(len);
  ser_.Off(len);
  r_.Off(len);
  g_.Off(len);
  b_.Off(len);
  qser_.Off(len);
  q_.Off(len);
}

IRAM void RMTChannelSet::Upload() {
  srclk_.Upload();
  ser_.Upload();
  r_.Upload();
  g_.Upload();
  b_.Upload();
  qser_.Upload();
  q_.Upload();
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
  // qa_.AttachPin();
  // qb_.AttachPin();
}

IRAM void RMTChannelSet::DetachQ() {
  // qa_.DetachPin();
  // qb_.DetachPin();
}

void RMTChannelSet::Dump() {
  srclk_.Dump();
  ser_.Dump();
  qser_.Dump();
  r_.Dump();
  g_.Dump();
  b_.Dump();
  q_.Dump();
  LOG(LL_INFO, ("%d+%d %d+%d %d+%d | %d+%d %d+%d %d+%d | %d+%d", srclk_.len_,
                srclk_.tot_len_, ser_.len_, ser_.tot_len_, qser_.len_,
                qser_.tot_len_, r_.len_, r_.tot_len_, g_.len_, g_.tot_len_,
                b_.len_, b_.tot_len_, q_.len_, q_.tot_len_));
}

}  // namespace clk
