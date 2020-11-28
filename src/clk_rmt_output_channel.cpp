/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_output_channel.hpp"

#include "mgos.hpp"

#include "driver/gpio.h"
#include "soc/rmt_reg.h"
#include "soc/rmt_struct.h"

namespace clk {

RMTOutputChannel::RMTOutputChannel(uint8_t ch, int pin, bool on_value,
                                   bool idle_value, bool loop,
                                   uint16_t carrier_high, uint16_t carrier_low,
                                   uint16_t tx_int_thresh)
    : RMTChannel(ch, pin, idle_value),
      on_value_(on_value),
      tx_int_thresh_(tx_int_thresh) {
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
  carrier_duty_reg_ = ((((uint32_t) carrier_high) << 16) | carrier_low);
  conf0_reg_ = ((1 << RMT_MEM_SIZE_CH0_S) | (80 << RMT_DIV_CNT_CH0_S));
  if (carrier_high > 0 && carrier_low > 0) {
    conf0_reg_ |= RMT_CARRIER_EN_CH0;
    if (on_value_) conf0_reg_ |= RMT_CARRIER_OUT_LV_CH0;
  }
}

void RMTOutputChannel::Init() {
  if (pin_ < 0) return;
  RMTChannel::Init();
  RMT.apb_conf.mem_tx_wrap_en = 1;
  RMT.conf_ch[ch_].conf0.val = conf0_reg_;
  RMT.carrier_duty_ch[ch_].val = carrier_duty_reg_;
  RMT.tx_lim_ch[ch_].limit = tx_int_thresh_;
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin_], PIN_FUNC_GPIO);
  gpio_set_direction((gpio_num_t) pin_, GPIO_MODE_OUTPUT);
  Attach();
  Stop();
}

IRAM void RMTOutputChannel::EnableInt() {
  uint32_t ch_int_mask = (RMT_CH0_TX_END_INT_ENA << ch_);
  if (tx_int_thresh_ > 0) {
    ch_int_mask |= (RMT_CH0_TX_THR_EVENT_INT_ENA << ch_);
  }
  RMT.int_ena.val |= ch_int_mask;
}

IRAM void RMTOutputChannel::Val(bool val, uint16_t num_cycles) {
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

IRAM void RMTOutputChannel::On(uint16_t num_cycles) {
  Val(on_value_, num_cycles);
}

IRAM void RMTOutputChannel::Off(uint16_t num_cycles) {
  Val(!on_value_, num_cycles);
}

IRAM void RMTOutputChannel::Set(bool on, uint16_t num_cycles) {
  Val((on ? on_value_ : !on_value_), num_cycles);
}

IRAM void RMTOutputChannel::OnTo(const RMTOutputChannel &other) {
  uint16_t diff = other.tot_len_ - tot_len_;
  if (diff == 0) return;
  Val(on_value_, diff);
}

IRAM void RMTOutputChannel::OffTo(const RMTOutputChannel &other) {
  uint16_t diff = other.tot_len_ - tot_len_;
  if (diff == 0) return;
  Val(!on_value_, diff);
}

void RMTOutputChannel::Clear() {
  len_ = 0;
  tot_len_ = 0;
}

IRAM void RMTOutputChannel::Upload() {
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

IRAM void RMTOutputChannel::Start() {
  RMT.conf_ch[ch_].conf1.val = conf1_start_;
}

IRAM void RMTOutputChannel::Stop() {
  RMTMEM.chan[ch_].data32[0].val = 0;
  RMT.conf_ch[ch_].conf1.val = conf1_stop_;
}

IRAM void RMTOutputChannel::Attach() {
  if (pin_ <= 0) return;
  gpio_matrix_out(pin_, RMT_SIG_OUT0_IDX + ch_, 0, 0);
  SetIntHandler(int_handler_, int_handler_arg_);
}

IRAM void RMTOutputChannel::Detach() {
  if (pin_ <= 0) return;
  DisableInt();
  SetIntHandler(nullptr, nullptr);
  gpio_matrix_out(pin_, SIG_GPIO_OUT_IDX, 0, 0);
}

}  // namespace clk
