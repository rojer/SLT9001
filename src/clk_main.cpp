/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <functional>

#include "mgos.hpp"
#include "mgos_app.h"
#include "mgos_timers.hpp"

#include "driver/periph_ctrl.h"
#include "soc/rmt_struct.h"

#include "clk_rmt_channel.hpp"

namespace clk {

static uint8_t s_symi = 0, s_cnt = 0;

static const uint8_t s_syms[16] = {
    0x03,  // 0000 0011, "0"
    0x9f,  // 1001 1111, "1"
    0x25,  // 0010 0101, "2"
    0x0d,  // 0000 1101, "3"
    0x99,  // 1001 1001, "4"
    0x49,  // 0100 1001, "5"
    0x41,  // 0100 0001, "6"
    0x1f,  // 0001 1111, "7"
    0x01,  // 0000 0001, "8"
    0x09,  // 0000 1001, "9"
    0x11,  // 0001 0001, "A"
    0xc1,  // 1100 0001, "b"
    0x63,  // 0110 0011, "C"
    0x85,  // 1000 0101, "d"
    0x61,  // 0110 0001, "E"
    0x71,  // 0111 0001, "F"
};

int s_digits[] = {Q1_GPIO, Q2_GPIO, Q5_GPIO, Q3_GPIO, Q4_GPIO};
int s_colors[] = {OE_R_GPIO, OE_G_GPIO, OE_B_GPIO};

static RMTChannel rmt_chans[8] = {
    {.ch = 0, .len = 0, .data = {}, .conf1_stop = 0, .conf1_run = 0},
    {.ch = 1, .len = 0, .data = {}, .conf1_stop = 0, .conf1_run = 0},
    {.ch = 2, .len = 0, .data = {}, .conf1_stop = 0, .conf1_run = 0},
    {.ch = 3, .len = 0, .data = {}, .conf1_stop = 0, .conf1_run = 0},
    {.ch = 4, .len = 0, .data = {}, .conf1_stop = 0, .conf1_run = 0},
    {.ch = 5, .len = 0, .data = {}, .conf1_stop = 0, .conf1_run = 0},
    {.ch = 6, .len = 0, .data = {}, .conf1_stop = 0, .conf1_run = 0},
    {.ch = 7, .len = 0, .data = {}, .conf1_stop = 0, .conf1_run = 0},
};

static RMTChannel *ser_ch, *srclk_ch, *rclk_ch;

static void GenDigitSeq2(uint8_t digit, uint16_t len, RMTChannel *ser,
                         RMTChannel *clk, RMTChannel *rclk) {
  rclk->AddInsn(0, len * 16);
  for (uint8_t mask = 1; mask != 0; mask <<= 1) {
    // Set bit value in SER
    ser->AddInsn((digit & mask) != 0, len * 2);
    // Latch into shift register
    clk->AddInsn(0, len);
    clk->AddInsn(1, len);
  }
  // Latch shift -> storage register.
  ser->AddInsn(0, len);
  clk->AddInsn(0, len);
  rclk->AddInsn(1, len);
}

void SendDigit(uint8_t digit) {
  rclk_ch->Stop();
  srclk_ch->Stop();
  ser_ch->Stop();
  GenDigitSeq2(digit, 1, ser_ch, srclk_ch, rclk_ch);
  ser_ch->CopyData();
  srclk_ch->CopyData();
  rclk_ch->CopyData();
  mgos_ints_disable();
  ser_ch->Start();
  srclk_ch->Start();
  rclk_ch->Start();
  mgos_ints_enable();
}

static void TimerCB(void *arg) {
  s_symi++;
  if (s_symi == 10) {
    s_symi = 0;
    s_cnt++;
    if (s_cnt == 3) s_cnt = 0;
  }
  SendDigit(s_syms[s_symi]);
  switch (s_cnt) {
    case 0:
      mgos_gpio_setup_output(OE_R_GPIO, 0);
      mgos_gpio_setup_output(OE_G_GPIO, 1);
      mgos_gpio_setup_output(OE_B_GPIO, 1);
      break;
    case 1:
      mgos_gpio_setup_output(OE_R_GPIO, 1);
      mgos_gpio_setup_output(OE_G_GPIO, 0);
      mgos_gpio_setup_output(OE_B_GPIO, 1);
      break;
    case 2:
      mgos_gpio_setup_output(OE_R_GPIO, 1);
      mgos_gpio_setup_output(OE_G_GPIO, 1);
      mgos_gpio_setup_output(OE_B_GPIO, 0);
      break;
  }
  LOG(LL_INFO, ("Hello %2d 0x%02x", s_symi, s_syms[s_symi]));
  (void) arg;
}

static mgos::ScopedTimer s_tmr(std::bind(TimerCB, nullptr));

void InitRMT() {
  periph_module_enable(PERIPH_RMT_MODULE);
  RMT.apb_conf.fifo_mask = 1;
  RMT.apb_conf.mem_tx_wrap_en = 0;
  ser_ch = &rmt_chans[SER_RMT_CH];
  ser_ch->Configure(SER_GPIO, 0);
  srclk_ch = &rmt_chans[SRCLK_RMT_CH];
  srclk_ch->Configure(SRCLK_GPIO, 0);
  rclk_ch = &rmt_chans[RCLK_RMT_CH];
  rclk_ch->Configure(RCLK_GPIO, 0);
}

bool InitApp() {
  mgos_gpio_setup_output(OE_R_GPIO, 0);
  mgos_gpio_setup_output(OE_G_GPIO, 1);
  mgos_gpio_setup_output(OE_B_GPIO, 1);
  mgos_gpio_setup_output(Q1_GPIO, 1);
  mgos_gpio_setup_output(Q2_GPIO, 1);
  mgos_gpio_setup_output(Q3_GPIO, 1);
  mgos_gpio_setup_output(Q4_GPIO, 1);
  mgos_gpio_setup_output(Q5_GPIO, 0);
  s_tmr.Reset(1000, MGOS_TIMER_REPEAT);

  InitRMT();

  return true;
}

}  // namespace clk

extern "C" {
enum mgos_app_init_result mgos_app_init(void) {
  return (clk::InitApp() ? MGOS_APP_INIT_SUCCESS : MGOS_APP_INIT_ERROR);
}
}
