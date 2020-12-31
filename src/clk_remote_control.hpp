/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include "mgos_event.h"

#define CLK_BTN_EV_BASE MGOS_EVENT_BASE('B', 'T', 'N')

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

enum class RemoteControlButtonEvent {
  kButtonDown = CLK_BTN_EV_BASE,
  kButtonRepeat,
  kButtonUp,
};

struct RemoteControlButtonDownEventArg {
  RemoteControlButton btn;
  bool repeat;
};
struct RemoteControlButtonUpEventArg {
  RemoteControlButton btn;
};

void RemoteControlInit();

}  // namespace clk
