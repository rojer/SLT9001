/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include "clk_rmt_channel.hpp"

namespace clk {

class RMTOutputChannel : public RMTChannel {
 public:
  RMTOutputChannel(uint8_t ch, int pin, bool on_value, bool idle_value,
                   bool loop = false, uint16_t carrier_high = 0,
                   uint16_t carrier_low = 0, uint16_t tx_int_thresh = 0);
  RMTOutputChannel(const RMTOutputChannel &other) = default;

  void Init() override;

  void EnableInt() override;

  // Methods to build the sequence.
  void Clear();
  void On(uint16_t num_cycles);
  void Off(uint16_t num_cycles);
  void Set(bool on, uint16_t num_cycles);
  void OnTo(const RMTOutputChannel &other);
  void OffTo(const RMTOutputChannel &other);

  // Upload the sequence from memory to the peripheral.
  void Upload();

  void Start() override;
  void Stop() override;

  // Attach/detach peripheral from the pin.
  void Attach() override;
  void Detach() override;

 private:
  void Val(bool val, uint16_t num_cycles);

  const bool on_value_;
  const uint16_t tx_int_thresh_;

  uint32_t carrier_duty_reg_ = 0;

  // Needs conf1_start to optimize starting channels.
  // friend void RMTOutputChannelSet::Start();
  friend class DisplayController;
};

}  // namespace clk
