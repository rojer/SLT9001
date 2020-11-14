/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include <stdint.h>

#include "soc/rmt_struct.h"

namespace clk {

struct RMTChannel {
  RMTChannel(uint8_t ch, int pin, bool idle_value);
  uint8_t ch;
  int pin;
  bool idle_value;
  uint32_t len;
  uint32_t tot_cycles;
  uint32_t conf1_stop, conf1_start;
  union {
    uint16_t data16[128];
    uint32_t data32[64];
  } data;

  void Setup();

  void On(uint16_t num_cycles);
  void Off(uint16_t num_cycles);
  void Val(bool val, uint16_t num_cycles);
  void OffTo(const RMTChannel &other);

  void ClearData();
  void LoadData();
  void DumpData();

  inline void Start() {
    RMT.conf_ch[ch].conf1.val = conf1_start;
  }
  inline void Stop() {
    RMTMEM.chan[ch].data32[0].val = 0;
    RMT.conf_ch[ch].conf1.val = conf1_stop;
  }
  void Attach();
  void Detach();
};

}  // namespace clk
