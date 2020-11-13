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
#include "mgos_rpc.h"
#include "mgos_timers.hpp"

#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
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
    RMTChannel(0), RMTChannel(1), RMTChannel(2), RMTChannel(3),
    RMTChannel(4), RMTChannel(5), RMTChannel(6), RMTChannel(7),
};

static RMTChannel *ser, *srclk, *rclk;
static RMTChannel *r, *g, *b;
static RMTChannel *qa, *qb;

static void GenDigitSeq(uint8_t digit, uint16_t len) {
  rclk->Val(0, len * 16);
  for (uint8_t mask = 1; mask != 0; mask <<= 1) {
    // Set bit value in SER
    ser->Val((digit & mask) != 0, len * 2);
    // Latch into shift register
    srclk->Off(len);
    srclk->On(len);
  }
  // Latch shift -> storage register.
  ser->Off(len);
  srclk->Off(len);
  rclk->On(len * 2);
  ser->OffTo(rclk);
  srclk->OffTo(rclk);
}

uint16_t rl = 1000, gl = 1000, bl = 1000, dl = 2000;

IRAM void SendDigits(uint8_t digit1, uint8_t digit2) {
  for (int i = 0; i < 8; i++) {
    rmt_chans[i].ClearData();
  }
#if 0  // Start calibration pattern
  rmt_chans[0].On(10);
  rmt_chans[0].Off(10);
  rmt_chans[0].On(1);
  rmt_chans[0].Off(1);
  rmt_chans[0].On(5);
  rmt_chans[0].Off(5);

  rmt_chans[7].On(10);
  rmt_chans[7].Off(10);
  rmt_chans[7].On(1);
  rmt_chans[7].Off(1);
  rmt_chans[7].On(5);
  rmt_chans[7].Off(5);
#else
  // Digit 1
  {  // Clock out digit data
    GenDigitSeq(digit1, 4);
    r->OffTo(rclk);
    g->OffTo(rclk);
    b->OffTo(rclk);
    qa->OffTo(rclk);
    qb->OffTo(rclk);
  }
  if (rl > 0) {  // 1.R Enable QA + R.
    r->On(rl);
    qa->On(rl);
    // Idle channels
    rclk->OffTo(r);
    srclk->OffTo(r);
    ser->OffTo(r);
    // r
    g->OffTo(r);
    b->OffTo(r);
    // qa
    qb->OffTo(r);
  }
  if (gl > 0) {  // 1.G Disable R, enable G.
    r->Off(gl);
    g->On(gl);
    qa->On(gl);
    // Idle channels
    rclk->OffTo(g);
    srclk->OffTo(g);
    ser->OffTo(g);
    // r
    // g
    b->OffTo(g);
    // qa
    qb->OffTo(g);
  }
  if (bl > 0) {  // 1.B Disable G, enable B.
    g->Off(bl);
    b->On(bl);
    qa->On(bl);
    // Idle channels
    rclk->OffTo(b);
    srclk->OffTo(b);
    ser->OffTo(b);
    r->OffTo(b);
    // g
    // b
    // qa
    qb->OffTo(b);
  }
  // Digit 2
  {  // Clock out digit data
    GenDigitSeq(digit2, 4);
    r->OffTo(rclk);
    g->OffTo(rclk);
    b->OffTo(rclk);
    qa->OffTo(rclk);
    qb->OffTo(rclk);
  }
  if (rl > 0) {  // 2.R Enable QA + R.
    r->On(rl);
    qb->On(rl);
    // Idle channels
    rclk->OffTo(r);
    srclk->OffTo(r);
    ser->OffTo(r);
    // r
    g->OffTo(r);
    b->OffTo(r);
    qa->OffTo(r);
    // qb
  }
  if (gl > 0) {  // 2.G Disable R, enable G.
    r->Off(gl);
    g->On(gl);
    qb->On(gl);
    // Idle channels
    rclk->OffTo(g);
    srclk->OffTo(g);
    ser->OffTo(g);
    // r
    // g
    b->OffTo(g);
    qa->OffTo(g);
    // qb
  }
  if (bl > 0) {  // 2.B Disable G, enable B.
    g->Off(bl);
    b->On(bl);
    qb->On(bl);
    // Idle channels
    rclk->OffTo(b);
    srclk->OffTo(b);
    ser->OffTo(b);
    r->OffTo(b);
    // g
    // b
    qa->OffTo(b);
    // qb
  }
  // Pause
  if (dl > 0) {
    rclk->Off(dl);
    srclk->Off(dl);
    ser->Off(dl);
    r->Off(dl);
    g->Off(dl);
    b->Off(dl);
    qa->Off(dl);
    qb->Off(dl);
  }
#endif
  // Time-critical part.
  mgos_ints_disable();
  // Make sure transition occurs when we are not loading the shift register.
  int wait = 0;
  const uint32_t int_mask =
      (RMT_CH0_TX_END_INT_RAW);  // | RMT_CH1_TX_END_INT_RAW |
                                 // RMT_CH2_TX_END_INT_RAW);
  RMT.int_clr.val = int_mask;
  for (int i = 0; i < 8; i++) {
    rmt_chans[i].Drain();
  }
  while (wait < 1000000) {
    if ((RMT.int_raw.val & int_mask) == int_mask) {
      RMT.conf_ch[0].conf1.tx_conti_mode = 0;
      break;
    }
    wait++;
  }
  for (int i = 0; i < 8; i++) {
    rmt_chans[i].Stop();
  }
  uint32_t start_values[8];
  for (int i = 0; i < 8; i++) {
    rmt_chans[i].CopyData();
    start_values[i] = rmt_chans[i].conf1_start;
  }
  // This is time critical, we must kick off all channels as close to
  // simultaneously as possible
#if 0
  for (int i = 0; i < 8; i++) {
    rmt_chans[i].Start();
  }
#else
  uint32_t sv = start_values[0];
  uint32_t sv2 = start_values[3];
  *((uint32_t *) RMT_CH0CONF1_REG) = sv;
  *((uint32_t *) RMT_CH1CONF1_REG) = sv;
  *((uint32_t *) RMT_CH2CONF1_REG) = sv;
  *((uint32_t *) RMT_CH3CONF1_REG) = sv2;
  *((uint32_t *) RMT_CH4CONF1_REG) = sv2;
  *((uint32_t *) RMT_CH5CONF1_REG) = sv2;
  *((uint32_t *) RMT_CH6CONF1_REG) = sv;
  *((uint32_t *) RMT_CH7CONF1_REG) = sv;
#endif
  mgos_ints_enable();
  LOG(LL_INFO,
      ("%d %d %d | %d %d %d | %d %d | %#08x | wait %d %#08x", ser->tot_cycles,
       srclk->tot_cycles, rclk->tot_cycles, r->tot_cycles, g->tot_cycles,
       b->tot_cycles, qa->tot_cycles, qb->tot_cycles, RMT.int_raw.val, wait,
       RMT.conf_ch[0].conf1.val));
#if 0
  for (int i = 0; i < 8; i++) {
    rmt_chans[i].DumpData();
  }
#endif
}

static void SetColorHandler(struct mg_rpc_request_info *ri, void *cb_arg,
                            struct mg_rpc_frame_info *fi, struct mg_str args) {
  int r = -1, g = -1, b = -1, d = -1;
  json_scanf(args.p, args.len, ri->args_fmt, &r, &g, &b, &d);
  if (r >= 0) {
    rl = r;
  }
  if (g >= 0) {
    gl = g;
  }
  if (b >= 0) {
    bl = b;
  }
  if (d >= 0) {
    dl = d;
  }
  mg_rpc_send_responsef(ri, nullptr);
}

static void TimerCB(void *arg) {
  // s_symi=1;
  s_symi++;
  if (s_symi == 9) {
    s_symi = 0;
    s_cnt++;
    if (s_cnt == 3) s_cnt = 0;
  }
  SendDigits(s_syms[2], s_syms[5]);
  // SendDigits(s_syms[s_symi], s_syms[s_symi + 1]);
  LOG(LL_INFO, ("Hello %2d 0x%02x", s_symi, s_syms[s_symi]));
  (void) arg;
}

static mgos::ScopedTimer s_tmr(std::bind(TimerCB, nullptr));

void InitRMT() {
  periph_module_enable(PERIPH_RMT_MODULE);
  RMT.apb_conf.fifo_mask = 1;
  RMT.apb_conf.mem_tx_wrap_en = 0;
  rclk = &rmt_chans[0];
  rclk->Configure(RCLK_GPIO, 0);
  srclk = &rmt_chans[1];
  srclk->Configure(SRCLK_GPIO, 0);
  ser = &rmt_chans[2];
  ser->Configure(SER_GPIO, 0);

  r = &rmt_chans[3];
  r->Configure(OE_R_GPIO, 1);
  g = &rmt_chans[4];
  g->Configure(OE_G_GPIO, 1);
  b = &rmt_chans[5];
  b->Configure(OE_B_GPIO, 1);

  qa = &rmt_chans[6];
  qa->Configure(Q1_GPIO, 0);
  qb = &rmt_chans[7];
  qb->Configure(Q2_GPIO, 0);
}

bool InitApp() {
  mgos_gpio_setup_output(OE_R_GPIO, 1);
  mgos_gpio_setup_output(OE_G_GPIO, 1);
  mgos_gpio_setup_output(OE_B_GPIO, 1);
  mgos_gpio_setup_output(Q1_GPIO, 1);
  mgos_gpio_setup_output(Q2_GPIO, 1);
  mgos_gpio_setup_output(Q3_GPIO, 0);
  mgos_gpio_setup_output(Q4_GPIO, 0);
  mgos_gpio_setup_output(Q5_GPIO, 0);
  s_tmr.Reset(1000, MGOS_TIMER_REPEAT);

  InitRMT();

  mg_rpc_add_handler(mgos_rpc_get_global(), "Clock.SetColor",
                     "{r: %d, g: %d, b: %d, d: %d}", SetColorHandler, nullptr);

  return true;
}

}  // namespace clk

extern "C" {
enum mgos_app_init_result mgos_app_init(void) {
  return (clk::InitApp() ? MGOS_APP_INIT_SUCCESS : MGOS_APP_INIT_ERROR);
}
}
