/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_channel.hpp"

#include "mgos.hpp"

namespace clk {

RMTChannel::RMTChannel(uint8_t ch, int pin, bool idle_value)
    : ch_(ch), pin_(pin), idle_value_(idle_value), data_({}) {
}

void RMTChannel::Dump() {
  LOG(LL_INFO, ("ch %d pin %d n %d tot_len %d conf0 %#08x carr %#08x start "
                "%#08x stop %#08x",
                ch_, pin_, len_, tot_len_, RMT.conf_ch[ch_].conf0.val,
                RMT.carrier_duty_ch[ch_].val, conf1_start_, conf1_stop_));
  mg_hexdumpf(stderr, (void *) &data_, len_ * 2);
}

}  // namespace clk
