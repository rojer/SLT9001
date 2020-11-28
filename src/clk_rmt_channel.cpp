/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_channel.hpp"

#include "mgos.hpp"

#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
#include "soc/rmt_struct.h"

namespace clk {

// static
intr_handle_t RMTChannel::inth_ = 0;
RMTChannel *RMTChannel::int_handler_objs_[RMT_NUM_CH] = {};

RMTChannel::RMTChannel(uint8_t ch, int pin, bool idle_value)
    : ch_(ch), pin_(pin), idle_value_(idle_value), data_({}) {
}

void RMTChannel::Init() {
  periph_module_enable(PERIPH_RMT_MODULE);
  RMT.apb_conf.fifo_mask = 1;
  DisableInt();
}

#define RMT_CH_INT_MASK(ch)                                                 \
  ((RMT_CH0_TX_THR_EVENT_INT_ENA << (ch)) |                                 \
   ((RMT_CH0_ERR_INT_ENA | RMT_CH0_RX_END_INT_ENA | RMT_CH0_TX_END_INT_ENA) \
    << ((ch) *3)))

IRAM void RMTChannel::DisableInt() {
  RMT.int_ena.val &= ~RMT_CH_INT_MASK(ch_);
}

IRAM void RMTChannel::ClearInt() {
  RMT.int_clr.val = RMT_CH_INT_MASK(ch_);
}

void RMTChannel::Dump() {
  LOG(LL_INFO, ("ch %d pin %d n %d tot_len %d conf0 %#08x carr %#08x start "
                "%#08x stop %#08x",
                ch_, pin_, len_, tot_len_, RMT.conf_ch[ch_].conf0.val,
                RMT.carrier_duty_ch[ch_].val, conf1_start_, conf1_stop_));
  mg_hexdumpf(stderr, (void *) &data_, len_ * 2);
}

IRAM void RMTChannel::SetIntHandler(void (*handler)(RMTChannel *obj, void *arg),
                                    void *arg) {
  int_handler_ = handler;
  int_handler_arg_ = arg;
  SetIntHandlerInternal(ch_, (handler != nullptr ? this : nullptr));
}

// static
IRAM void RMTChannel::SetIntHandlerInternal(uint8_t ch, RMTChannel *obj) {
  int_handler_objs_[ch] = obj;
  if (inth_ == 0) {
    esp_intr_alloc(ETS_RMT_INTR_SOURCE, 0, RMTIntHandler, nullptr, &inth_);
    esp_intr_set_in_iram(inth_, true);
  }
}

// static
IRAM void RMTChannel::RMTIntHandler(void *arg) {
  uint32_t int_st = RMT.int_st.val;
  uint32_t int_mask1 = RMT_CH0_TX_THR_EVENT_INT_ST;
  uint32_t int_mask2 =
      (RMT_CH0_ERR_INT_ST | RMT_CH0_RX_END_INT_ST | RMT_CH0_TX_END_INT_ST);
  for (size_t ch = 0; ch < RMT_NUM_CH; ch++, int_mask1 <<= 1, int_mask2 <<= 3) {
    uint32_t ch_int_mask = int_mask1 | int_mask2;
    if ((int_st & ch_int_mask) == 0) continue;
    RMT.int_clr.val = ch_int_mask;
    RMTChannel *obj = int_handler_objs_[ch];
    if (obj == nullptr || obj->int_handler_ == nullptr) continue;
    obj->int_handler_(obj, obj->int_handler_arg_);
  }
  (void) arg;
}

}  // namespace clk
