/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include <stdint.h>

namespace clk {

struct RMTChannel {
  uint8_t ch;
  uint8_t len;
  union {
    uint16_t data16[128];
    uint32_t data32[64];
  } data;
  uint32_t conf1_stop, conf1_run;

  void Configure(int gpio, bool idle_value);
  void AddInsn(bool val, uint16_t num_cycles);
  void CopyData();
  void Start();
  void Stop();
};

}  // namespace clk
