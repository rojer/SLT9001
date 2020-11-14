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

#define LOOP 0

RMTChannel::RMTChannel(uint8_t ch, int pin, bool idle_value)
    : ch(ch),
      pin(pin),
      idle_value(idle_value),
      len(0),
      conf1_stop(0),
      conf1_start(0),
      data({}) {
}

void RMTChannel::Setup() {
  if (pin < 0) return;
  this->idle_value = idle_value;
  RMT.conf_ch[ch].conf0.val =
      ((1 << RMT_MEM_SIZE_CH0_S) | (80 << RMT_DIV_CNT_CH0_S));
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin], 2);
  gpio_set_direction((gpio_num_t) pin, GPIO_MODE_OUTPUT);
  if (idle_value) {
    conf1_stop =
        (RMT_IDLE_OUT_EN_CH0 | RMT_IDLE_OUT_LV_CH0 | RMT_REF_CNT_RST_CH0 |
         RMT_MEM_RD_RST_CH0 | RMT_REF_ALWAYS_ON_CH0);
    conf1_start =
        (RMT_IDLE_OUT_EN_CH0 | RMT_IDLE_OUT_LV_CH0 |
         (RMT_TX_CONTI_MODE_CH0 * LOOP) | RMT_REF_CNT_RST_CH0 |
         RMT_MEM_RD_RST_CH0 | RMT_TX_START_CH0 | RMT_REF_ALWAYS_ON_CH0);
  } else {
    conf1_stop = (RMT_IDLE_OUT_EN_CH0 | RMT_REF_CNT_RST_CH0 |
                  RMT_MEM_RD_RST_CH0 | RMT_REF_ALWAYS_ON_CH0);
    conf1_start = (RMT_IDLE_OUT_EN_CH0 | (RMT_TX_CONTI_MODE_CH0 * LOOP) |
                   RMT_REF_CNT_RST_CH0 | RMT_MEM_RD_RST_CH0 | RMT_TX_START_CH0 |
                   RMT_REF_ALWAYS_ON_CH0);
  }
  Attach();
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

IRAM void RMTChannel::OffTo(const RMTChannel &other) {
  uint16_t diff = other.tot_cycles - tot_cycles;
  if (diff == 0) return;
  Val(false, diff);
}

void RMTChannel::ClearData() {
  len = 0;
  tot_cycles = 0;
}

IRAM void RMTChannel::LoadData() {
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

IRAM void RMTChannel::Attach() {
  if (pin <= 0) return;
  gpio_matrix_out(pin, RMT_SIG_OUT0_IDX + ch, 0, 0);
}

IRAM void RMTChannel::Detach() {
  if (pin <= 0) return;
  gpio_matrix_out(pin, SIG_GPIO_OUT_IDX, 0, 0);
}

}  // namespace clk
