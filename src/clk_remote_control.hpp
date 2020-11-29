/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

namespace clk {

enum class RemoteControlButton {
  kNone = 0,
  kOnOff = 1,
  kDP = 2,
  kUp = 3,
  kSet = 4,
  kDown = 5,
  kReset = 6,
  kMax,
};

void RemoteControlInit();

}  // namespace clk
