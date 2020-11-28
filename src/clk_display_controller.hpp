/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include <cstdint>

#include "clk_rmt_output_channel.hpp"

namespace clk {

class DisplayController {
 public:
  explicit DisplayController(void(int_handler)());
  DisplayController(const DisplayController &other) = default;

  void Init();

  static constexpr uint8_t kDigitValueEmpty = 0xff;

  void SetDigits(uint8_t digits[5], uint16_t rl, uint16_t gl, uint16_t bl,
                 uint16_t dl);

  // Data generation functions.
  void Clear();
  void GenDigitSeq(uint8_t qn, uint8_t d, uint16_t len, uint16_t rl,
                   uint16_t gl, uint16_t bl, uint16_t dl);
  void GenIdleSeq(uint16_t dl);

  void Upload();
  void Attach();
  void Detach();
  void Start();

  void Dump();

 private:
  static void ChannelIntHandler(RMTChannel *ch, void *arg);

  RMTOutputChannel srclk_, ser_, qser_, rclk_;
  RMTOutputChannel r_, g_, b_;
  void (*int_handler_)();
};

void SetDisplayDigits(uint8_t digits[5], uint16_t rl, uint16_t gl, uint16_t bl,
                      uint16_t dl);

}  // namespace clk
