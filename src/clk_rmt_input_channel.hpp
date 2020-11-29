/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include "clk_rmt_channel.hpp"

namespace clk {

class RMTInputChannel : public RMTChannel {
 public:
  RMTInputChannel(uint8_t ch, int pin, bool idle_value, uint8_t filter_thresh,
                  uint16_t idle_thresh);
  RMTInputChannel(const RMTInputChannel &other) = default;

  void Init() override;

  void EnableInt() override;

  // Start/stop the channel.
  void Start() override;
  void Stop() override;

  // Attach/detach peripheral from the pin.
  void Attach() override;
  void Detach() override;

 private:
};

}  // namespace clk
