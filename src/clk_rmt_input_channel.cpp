/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_input_channel.hpp"

#include "mgos.hpp"

#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
#include "soc/rmt_struct.h"

namespace clk {

RMTInputChannel::RMTInputChannel(uint8_t ch, int pin, bool idle_value,
                                 uint8_t filter_thresh, uint16_t idle_thresh)
    : RMTChannel(ch, pin, idle_value) {
  uint32_t conf1_common = (RMT_REF_ALWAYS_ON_CH0 | RMT_MEM_OWNER_CH0 |
                           RMT_REF_CNT_RST_CH0 | RMT_MEM_WR_RST_CH0);
  if (filter_thresh > 0) {
    conf1_common |= (filter_thresh << RMT_RX_FILTER_THRES_CH0_S);
    conf1_common |= RMT_RX_FILTER_EN_CH0;
  }
  conf1_stop_ = conf1_common;
  conf1_start_ = (conf1_common | RMT_RX_EN_CH0);
  conf0_reg_ = ((1 << RMT_MEM_SIZE_CH0_S) | (80 << RMT_DIV_CNT_CH0_S));
  if (idle_thresh == 0) idle_thresh = 0x1000;
  conf0_reg_ |= (idle_thresh << RMT_IDLE_THRES_CH0_S);
}

void RMTInputChannel::Init() {
  if (pin_ < 0) return;
  RMTChannel::Init();
  RMT.conf_ch[ch_].conf0.val = conf0_reg_;
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin_], PIN_FUNC_GPIO);
  gpio_set_direction((gpio_num_t) pin_, GPIO_MODE_INPUT);
  Stop();
}

IRAM void RMTInputChannel::EnableInt() {
  uint32_t ch_int_mask = (RMT_CH0_RX_END_INT_ENA << (ch_ * 3));
  RMT.int_ena.val |= ch_int_mask;
}

IRAM void RMTInputChannel::Start() {
  RMT.conf_ch[ch_].conf1.val = conf1_start_;
}

IRAM void RMTInputChannel::Stop() {
  RMT.conf_ch[ch_].conf1.val = conf1_stop_;
}

IRAM void RMTInputChannel::Attach() {
  if (pin_ <= 0) return;
  gpio_matrix_in(pin_, RMT_SIG_IN0_IDX + ch_, false /* inv */);
  ClearInt();
  SetIntHandlerInternal(ch_, this);
}

IRAM void RMTInputChannel::Detach() {
  if (pin_ <= 0) return;
  DisableInt();
  SetIntHandlerInternal(ch_, nullptr);
  gpio_matrix_in(pin_, SIG_GPIO_OUT_IDX, false /* inv */);
}

}  // namespace clk
