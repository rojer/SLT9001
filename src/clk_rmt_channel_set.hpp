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
  RMTChannelSet();
  RMTChannelSet(const RMTChannelSet &other) = default;

  void Init();

  // Data generation functions.
  void Clear();
  static constexpr uint8_t kDigitValueEmpty = 0xff;
  void GenDigitSeq(uint8_t qn, uint8_t d, uint16_t len, uint16_t rl,
                   uint16_t gl, uint16_t bl, uint16_t dl);

  void Upload();
  void Start();

  void Dump();

 private:
  RMTChannel srclk_, ser_, qser_, rclk_;
  RMTChannel r_, g_, b_;
};

}  // namespace clk