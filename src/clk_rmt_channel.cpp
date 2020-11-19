/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_channel.hpp"

#include "mgos.hpp"

#include "driver/gpio.h"
#include "soc/rmt_reg.h"
#include "soc/rmt_struct.h"

namespace clk {

RMTChannel::RMTChannel(uint8_t ch, int pin, bool on_value, bool idle_value,
                       bool loop)
    : ch_(ch),
      pin_(pin),
      on_value_(on_value),
      idle_value_(idle_value),
      data_({}) {
  uint32_t conf1_common = (RMT_IDLE_OUT_EN_CH0 | RMT_REF_ALWAYS_ON_CH0 |
                           RMT_REF_CNT_RST_CH0 | RMT_MEM_RD_RST_CH0);
  if (idle_value_) {
    conf1_common |= RMT_IDLE_OUT_LV_CH0;
  }
  if (loop) {
    conf1_common |= RMT_TX_CONTI_MODE_CH0;
  }
  conf1_stop_ = conf1_common;
  conf1_start_ = (conf1_common | RMT_TX_START_CH0);
}

void RMTChannel::Init() {
  if (pin_ < 0) return;
  RMT.conf_ch[ch_].conf0.val =
      ((1 << RMT_MEM_SIZE_CH0_S) | (80 << RMT_DIV_CNT_CH0_S));
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin_], 2);
  gpio_set_direction((gpio_num_t) pin_, GPIO_MODE_OUTPUT);
  Stop();
  AttachPin();
}

IRAM void RMTChannel::Val(bool val, uint16_t num_cycles) {
  if (num_cycles == 0) return;
  tot_len_ += num_cycles;
  if (len_ > 0) {
    Item *last = &data_.items[len_ - 1];
    if (val == last->val) {
      uint16_t avail = 0x7fff - last->num_cycles;
      if (avail >= num_cycles) {
        last->num_cycles += num_cycles;
        return;
      } else {
        LOG(LL_ERROR,
            ("%d: overflow %d %d", ch_, last->num_cycles, num_cycles));
        abort();
        last->num_cycles = 0x7fff;
        num_cycles -= avail;
      }
    }
  }
  Item *next = &data_.items[len_];
  next->num_cycles = num_cycles;
  next->val = val;
  len_++;
}

IRAM void RMTChannel::On(uint16_t num_cycles) {
  Val(on_value_, num_cycles);
}

IRAM void RMTChannel::Off(uint16_t num_cycles) {
  Val(!on_value_, num_cycles);
}

IRAM void RMTChannel::OnTo(const RMTChannel &other) {
  uint16_t diff = other.tot_len_ - tot_len_;
  if (diff == 0) return;
  Val(on_value_, diff);
}

IRAM void RMTChannel::OffTo(const RMTChannel &other) {
  uint16_t diff = other.tot_len_ - tot_len_;
  if (diff == 0) return;
  Val(!on_value_, diff);
}

void RMTChannel::Clear() {
  len_ = 0;
  tot_len_ = 0;
}

IRAM void RMTChannel::Upload() {
  const uint32_t *src = data_.data32;
  uint32_t *dst = (uint32_t *) &RMTMEM.chan[ch_].data32[0].val;
  for (int num_words = len_ / 2; num_words > 0; num_words--) {
    *dst++ = *src++;
  }
  if (len_ % 2 == 0) {
    *dst = (((uint32_t) idle_value_) << 15);
  } else {
    *dst = (*src & 0xffff) | (((uint32_t) idle_value_) << 31);
  }
}

void RMTChannel::Dump() {
  LOG(LL_INFO, ("ch %d pin %d len %d start %#08x stop %#08x", ch_, pin_, len_,
                conf1_start_, conf1_stop_));
  mg_hexdumpf(stderr, (void *) &data_, len_ * 2);
}

IRAM void RMTChannel::Start() {
  RMT.conf_ch[ch_].conf1.val = conf1_start_;
}

IRAM void RMTChannel::Stop() {
  RMTMEM.chan[ch_].data32[0].val = 0;
  RMT.conf_ch[ch_].conf1.val = conf1_stop_;
}

IRAM void RMTChannel::AttachPin() {
  if (pin_ <= 0) return;
  gpio_matrix_out(pin_, RMT_SIG_OUT0_IDX + ch_, 0, 0);
}

IRAM void RMTChannel::DetachPin() {
  if (pin_ <= 0) return;
  gpio_matrix_out(pin_, SIG_GPIO_OUT_IDX, 0, 0);
}

}  // namespace clk
