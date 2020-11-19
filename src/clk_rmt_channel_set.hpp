/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include <cstdint>

#include "clk_rmt_channel.hpp"

namespace clk {

class RMTChannelSet {
 public:
  RMTChannelSet(int qa_pin, int qb_pin);
  RMTChannelSet(const RMTChannelSet &other) = default;

  void Init();

  // Data generation functions.
  void Clear();
  void GenDigitA(uint8_t d, uint16_t rl, uint16_t gl, uint16_t bl);
  void GenDigitB(uint8_t d, uint16_t rl, uint16_t gl, uint16_t bl);
  void GenIdleSeq(uint16_t len);

  void Upload();
  void Start();

  void AttachQ();
  void DetachQ();

  void Dump();

 private:
  void GenDigitDataSeq(uint8_t digit, uint16_t len);
  void GenDigitControlSeq(RMTChannel *qa, RMTChannel *qb, uint16_t rl,
                          uint16_t gl, uint16_t bl);

  RMTChannel srclk_, ser_;
  RMTChannel r_, g_, b_;
  RMTChannel qa_, qb_;
};

}  // namespace clk
