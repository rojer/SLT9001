/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#pragma once

#include <stdint.h>

#include "esp_intr_alloc.h"
#include "soc/rmt_struct.h"

#define RMT_NUM_CH (sizeof(RMT.conf_ch) / sizeof(RMT.conf_ch[0]))

namespace clk {

class RMTChannel {
 public:
  RMTChannel(uint8_t ch, int pin, bool idle_value);
  RMTChannel(const RMTChannel &other) = default;

  virtual void Init();

  struct Item {
    uint16_t num_cycles : 15;
    uint16_t val : 1;
  } __attribute__((packed));

  const Item *data() const;
  size_t len() const;

  // Clear the data buffer and/or peripheral memory.
  void Clear(bool buf = true, bool mem = false);
  // Upload the sequence from the buffer to the peripheral.
  void Upload();
  // Download the sequence from the peripheral memory to the buffer.
  void Download();

  // Interrupt control.
  virtual void EnableInt() = 0;
  void DisableInt();
  void ClearInt();

  void SetIntHandler(void (*handler)(RMTChannel *obj, void *arg), void *arg);

  // Start/stop the channel.
  virtual void Start() = 0;
  virtual void Stop() = 0;

  // Attach/detach peripheral from the pin and int handler.
  virtual void Attach() = 0;
  virtual void Detach() = 0;

  // Dump current sequence in the buffer.
  void Dump();

 protected:
  const uint8_t ch_;
  const int pin_;
  const bool idle_value_;

  uint32_t len_ = 0;
  uint32_t tot_len_ = 0;
  uint32_t conf0_reg_ = 0;
  uint32_t conf1_start_ = 0;
  uint32_t conf1_stop_ = 0;

  union {
    Item items[128];
    uint32_t data32[64];
  } __attribute__((packed)) data_;

  static void SetIntHandlerInternal(uint8_t ch, RMTChannel *obj);

  void (*int_handler_)(RMTChannel *, void *arg) = nullptr;
  void *int_handler_arg_ = nullptr;

 private:
  static intr_handle_t inth_;

  static RMTChannel *int_handler_objs_[RMT_NUM_CH];

  static void RMTIntHandler(void *arg);
};

}  // namespace clk
