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
  RMTChannel(uint8_t ch, int pin, bool on_value, bool idle_value,
             bool loop = false, uint16_t carrier_high = 0,
             uint16_t carrier_low = 0);
  RMTChannel(const RMTChannel &other) = default;

  void Init();

  // Methods to build the sequence.
  void Clear();
  void On(uint16_t num_cycles);
  void Off(uint16_t num_cycles);
  void Set(bool on, uint16_t num_cycles);
  void OnTo(const RMTChannel &other);
  void OffTo(const RMTChannel &other);

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
  void Val(bool val, uint16_t num_cycles);

  uint8_t ch_;
  int pin_;
  bool on_value_;
  bool idle_value_;
  uint32_t len_ = 0;
  uint32_t tot_len_ = 0;
  uint32_t conf0_reg_ = 0;
  uint32_t carrier_duty_reg_ = 0;
  uint32_t conf1_start_ = 0;
  uint32_t conf1_stop_ = 0;
  struct Item {
    uint16_t num_cycles : 15;
    uint16_t val : 1;
  } __attribute__((packed));
  union {
    Item items[128];
    uint32_t data32[64];
  } __attribute__((packed)) data_;

  // Needs conf1_start to optimize starting channels.
  // friend void RMTChannelSet::Start();
  friend class RMTChannelSet;
};

}  // namespace clk
