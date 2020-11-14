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

  void Init();

  // Methods to build the sequence.
  void Clear();
  void On(uint16_t num_cycles);
  void Off(uint16_t num_cycles);
  void OnTo(const RMTChannel &other);
  void OffTo(const RMTChannel &other);
  void Val(bool val, uint16_t num_cycles);

  // Upload the sequence from memory to the peripheral.
  void Upload();

  // Start/stop the channel.
  void Start();
  void Stop();

  // Attach/detach peripheral from the pin.
  void AttachPin();
  void DetachPin();

  // Dump current sequence in the peripheral memory.
  void Dump();

 private:
  uint8_t ch_;
  int pin_;
  bool idle_value_;
  uint32_t len_ = 0;
  uint32_t tot_len_ = 0;
  uint32_t conf1_start_ = 0;
  uint32_t conf1_stop_ = 0;
  union {
    uint16_t data16[128];
    uint32_t data32[64];
  } data_;

  // Needs conf1_start to optimize starting channels.
  // friend void RMTChannelSet::Start();
  friend class RMTChannelSet;
};

}  // namespace clk
