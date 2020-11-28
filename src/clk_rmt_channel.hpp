/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include <stdint.h>

#include "soc/rmt_struct.h"

namespace clk {

class RMTChannel {
 public:
  RMTChannel(uint8_t ch, int pin, bool idle_value);
  RMTChannel(const RMTChannel &other) = default;

  virtual void Init() = 0;

  // Start/stop the channel.
  virtual void Start() = 0;
  virtual void Stop() = 0;

  // Attach/detach peripheral from the pin.
  virtual void AttachPin() = 0;
  virtual void DetachPin() = 0;

  // Dump current sequence in the buffer.
  void Dump();

  struct Item {
    uint16_t num_cycles : 15;
    uint16_t val : 1;
  } __attribute__((packed));

 protected:
  const uint8_t ch_;
  const int pin_;
  const bool idle_value_;

  uint32_t len_ = 0;
  uint32_t tot_len_ = 0;
  uint32_t conf0_reg_ = 0;
  uint32_t conf1_start_ = 0;
  uint32_t conf1_stop_ = 0;

  union {
    Item items[128];
    uint32_t data32[64];
  } __attribute__((packed)) data_;
};

}  // namespace clk
