/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include <stdint.h>

#include "soc/rmt_struct.h"

namespace clk {

struct RMTChannel {
  RMTChannel(uint8_t ch)
      : ch(ch), len(0), conf1_stop(0), conf1_start(0), data({}) {
  }

  uint8_t ch;
  uint8_t len;
  bool idle_value;
  uint32_t tot_cycles;
  uint32_t conf1_stop, conf1_start, conf1_drain;
  union {
    uint16_t data16[128];
    uint32_t data32[64];
  } data;

  void Configure(int gpio, bool idle_value);

  void On(uint16_t num_cycles);
  void Off(uint16_t num_cycles);
  void Val(bool val, uint16_t num_cycles);
  void OffTo(const RMTChannel *other);
  void ContinueTo(const RMTChannel *other);

  void ClearData();
  void DumpData();
  void CopyData();
  inline void Start() {
    RMT.conf_ch[ch].conf1.val = conf1_start;
  }
  inline void Drain() {
    RMT.conf_ch[ch].conf1.val = conf1_drain;
  }
  inline void Stop() {
    RMTMEM.chan[ch].data32[0].val = 0;
    RMT.conf_ch[ch].conf1.val = conf1_stop;
  }
};

}  // namespace clk
