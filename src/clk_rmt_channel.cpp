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

#define LOOP 1

IRAM void RMTChannel::Configure(int gpio, bool idle_value) {
  this->idle_value = idle_value;
  RMT.conf_ch[ch].conf0.val =
      ((1 << RMT_MEM_SIZE_CH0_S) | (80 << RMT_DIV_CNT_CH0_S));
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[gpio], 2);
  gpio_set_direction((gpio_num_t) gpio, GPIO_MODE_OUTPUT);
  gpio_matrix_out(gpio, RMT_SIG_OUT0_IDX + ch, 0, 0);
  if (idle_value) {
    conf1_stop =
        (RMT_IDLE_OUT_EN_CH0 | RMT_IDLE_OUT_LV_CH0 | RMT_REF_CNT_RST_CH0 |
         RMT_MEM_RD_RST_CH0 | RMT_REF_ALWAYS_ON_CH0);
    conf1_start =
        (RMT_IDLE_OUT_EN_CH0 | RMT_IDLE_OUT_LV_CH0 |
         (RMT_TX_CONTI_MODE_CH0 * LOOP) | RMT_REF_CNT_RST_CH0 |
         RMT_MEM_RD_RST_CH0 | RMT_TX_START_CH0 | RMT_REF_ALWAYS_ON_CH0);
    conf1_drain =
        (RMT_IDLE_OUT_EN_CH0 | RMT_IDLE_OUT_LV_CH0 | RMT_REF_ALWAYS_ON_CH0);
  } else {
    conf1_stop = (RMT_IDLE_OUT_EN_CH0 | RMT_REF_CNT_RST_CH0 |
                  RMT_MEM_RD_RST_CH0 | RMT_REF_ALWAYS_ON_CH0);
    conf1_start = (RMT_IDLE_OUT_EN_CH0 | (RMT_TX_CONTI_MODE_CH0 * LOOP) |
                   RMT_REF_CNT_RST_CH0 | RMT_MEM_RD_RST_CH0 | RMT_TX_START_CH0 |
                   RMT_REF_ALWAYS_ON_CH0);
    conf1_drain = (RMT_IDLE_OUT_EN_CH0 | RMT_REF_ALWAYS_ON_CH0);
  }
  Stop();
}

IRAM void RMTChannel::Val(bool val, uint16_t num_cycles) {
  tot_cycles += num_cycles;
  uint16_t v = (val ? !idle_value : idle_value);
  data.data16[len++] = ((v << 15) | num_cycles);
}

IRAM void RMTChannel::On(uint16_t num_cycles) {
  Val(true, num_cycles);
}

IRAM void RMTChannel::Off(uint16_t num_cycles) {
  Val(false, num_cycles);
}

IRAM void RMTChannel::OffTo(const RMTChannel *other) {
  uint16_t diff = other->tot_cycles - tot_cycles;
  if (diff == 0) return;
  Val(false, diff);
}

IRAM void RMTChannel::ContinueTo(const RMTChannel *other) {
  uint16_t diff = other->tot_cycles - tot_cycles;
  if (diff == 0) return;
  bool cur_value = idle_value;
  if (len > 0) {
    cur_value = (bool(data.data16[len - 1] & 0x8000) != idle_value);
  }
  Val(cur_value, diff);
}

IRAM void RMTChannel::ClearData() {
  len = 0;
  tot_cycles = 0;
}

IRAM void RMTChannel::CopyData() {
  const uint32_t *src = data.data32;
  volatile uint32_t *dst = &RMTMEM.chan[ch].data32[0].val;
  for (int num_words = len / 2; num_words > 0; num_words--) {
    *dst++ = *src++;
  }
  if (len % 2 == 0) {
    *dst = 0;
  } else {
    *dst = (*src & 0xffff);
  }
}

void RMTChannel::DumpData() {
  LOG(LL_INFO, ("ch %d len %d", ch, len));
  mg_hexdumpf(stderr, (void *) &RMTMEM.chan[ch].data32[0].val, len * 2 + 2);
}

}  // namespace clk
