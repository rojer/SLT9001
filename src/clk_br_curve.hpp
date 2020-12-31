/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include <cstdint>

namespace clk {

struct BrightnessCurveEntry {
  int8_t pct;
  uint16_t dl;
  float sf;
};

BrightnessCurveEntry GetBrightnessCurveEntry(int br_pct);

}  // namespace clk
