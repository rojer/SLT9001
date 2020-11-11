/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_rmt_channel.hpp"

#include "driver/gpio.h"
#include "soc/rmt_reg.h"
#include "soc/rmt_struct.h"

namespace clk {

IRAM void RMTChannel::Configure(int gpio, bool idle_value) {
  RMT.conf_ch[ch].conf0.val =
      ((1 << RMT_MEM_SIZE_CH0_S) | (1 << RMT_DIV_CNT_CH0_S));
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[gpio], 2);
  gpio_set_direction((gpio_num_t) gpio, GPIO_MODE_OUTPUT);
  gpio_matrix_out(gpio, RMT_SIG_OUT0_IDX + ch, 0, 0);
  if (idle_value) {
    conf1_stop = (RMT_IDLE_OUT_EN_CH0 | RMT_IDLE_OUT_LV_CH0 |
                  RMT_REF_CNT_RST_CH0 | RMT_MEM_RD_RST_CH0);
    conf1_run = (RMT_IDLE_OUT_EN_CH0 | RMT_IDLE_OUT_LV_CH0 | RMT_TX_START_CH0);
  } else {
    conf1_stop =
        (RMT_IDLE_OUT_EN_CH0 | RMT_REF_CNT_RST_CH0 | RMT_MEM_RD_RST_CH0);
    conf1_run = (RMT_IDLE_OUT_EN_CH0 | RMT_TX_START_CH0);
  }
  Stop();
}

IRAM void RMTChannel::AddInsn(bool val, uint16_t num_cycles) {
  data.data16[len++] = (((uint16_t) val) << 15) | num_cycles;
}

IRAM void RMTChannel::CopyData() {
  const uint32_t *src = data.data32;
  uint32_t *dst = (uint32_t *) &RMTMEM.chan[ch].data32[0].val;
  for (int num_words = len / 2; num_words > 0; num_words--) {
    *dst++ = *src++;
  }
  if (len % 2 == 0) {
    *dst = 0;
  } else {
    *dst = (*src & 0xffff);
  }
}

IRAM void RMTChannel::Start() {
  RMT.conf_ch[ch].conf1.val = conf1_run;
}

IRAM void RMTChannel::Stop() {
  len = 0;
  RMT.conf_ch[ch].conf1.val = conf1_stop;
}

}  // namespace clk
