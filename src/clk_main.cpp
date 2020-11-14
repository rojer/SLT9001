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
#include "rom/ets_sys.h"
#include "soc/rmt_reg.h"
#include "soc/rmt_struct.h"

#include "clk_rmt_channel.hpp"

namespace clk {

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

struct RMTChannelBank {
  RMTChannelBank(int qa_pin, int qb_pin)
      : ser(RMTChannel(0, SER_GPIO, 0)),
        srclk(RMTChannel(1, SRCLK_GPIO, 0)),
        rclk(RMTChannel(2, RCLK_GPIO, 0)),
        r(RMTChannel(3, OE_R_GPIO, 1)),
        g(RMTChannel(4, OE_G_GPIO, 1)),
        b(RMTChannel(5, OE_B_GPIO, 1)),
        qa(RMTChannel(6, qa_pin, 0)),
        qb(RMTChannel(7, qb_pin, 0)) {
  }

  RMTChannel ser, srclk, rclk;
  RMTChannel r, g, b;
  RMTChannel qa, qb;

  void GenDigitDataSeq(uint8_t digit, uint16_t len) {
    rclk.Val(0, len * 16);
    for (uint8_t mask = 1; mask != 0; mask <<= 1) {
      // Set bit value in SER
      ser.Val((digit & mask) != 0, len * 2);
      // Latch into shift register
      srclk.Off(len);
      srclk.On(len);
    }
    // Latch shift -> storage register.
    ser.Off(len);
    srclk.Off(len);
    rclk.On(len * 2);
    ser.OffTo(rclk);
    srclk.OffTo(rclk);
    r.OffTo(rclk);
    g.OffTo(rclk);
    b.OffTo(rclk);
    qa.OffTo(rclk);
    qb.OffTo(rclk);
  }

  void GenDigitControlSeq(RMTChannel *qa, RMTChannel *qb, uint16_t rl,
                          uint16_t gl, uint16_t bl) {
    if (rl > 0) {  // 1.R Enable QA + R.
      r.On(rl);
      qa->On(rl);
      // Idle channels
      rclk.OffTo(r);
      srclk.OffTo(r);
      ser.OffTo(r);
      // r
      g.OffTo(r);
      b.OffTo(r);
      // qa
      qb->OffTo(r);
    }
    if (gl > 0) {  // 1.G Disable R, enable G.
      r.Off(gl);
      g.On(gl);
      qa->On(gl);
      // Idle channels
      rclk.OffTo(g);
      srclk.OffTo(g);
      ser.OffTo(g);
      // r
      // g
      b.OffTo(g);
      // qa
      qb->OffTo(g);
    }
    if (bl > 0) {  // 1.B Disable G, enable B.
      g.Off(bl);
      b.On(bl);
      qa->On(bl);
      // Idle channels
      rclk.OffTo(b);
      srclk.OffTo(b);
      ser.OffTo(b);
      r.OffTo(b);
      // g
      // b
      // qa
      qb->OffTo(b);
    }
  }

  void GenIdleSeq(uint16_t len) {
    rclk.Off(len);
    srclk.Off(len);
    ser.Off(len);
    r.Off(len);
    g.Off(len);
    b.Off(len);
    qa.Off(len);
    qb.Off(len);
  }

  void GenDigitA(uint8_t d, uint16_t rl, uint16_t gl, uint16_t bl) {
    GenDigitDataSeq(d, 2);
    GenDigitControlSeq(&qa, &qb, rl, gl, bl);
  }

  void GenDigitB(uint8_t d, uint16_t rl, uint16_t gl, uint16_t bl) {
    GenDigitDataSeq(d, 2);
    GenDigitControlSeq(&qb, &qa, rl, gl, bl);
  }

  void Setup() {
    ser.Setup();
    srclk.Setup();
    rclk.Setup();
    r.Setup();
    g.Setup();
    b.Setup();
    qa.Setup();
    qb.Setup();
  }

  void ClearData() {
    ser.ClearData();
    srclk.ClearData();
    rclk.ClearData();
    r.ClearData();
    g.ClearData();
    b.ClearData();
    qa.ClearData();
    qb.ClearData();
  }

  inline void Stop() {
    rclk.Stop();
    srclk.Stop();
    ser.Stop();
    r.Stop();
    g.Stop();
    b.Stop();
    qa.Stop();
    qb.Stop();
  }

  inline void LoadData() {
    rclk.LoadData();
    srclk.LoadData();
    ser.LoadData();
    r.LoadData();
    g.LoadData();
    b.LoadData();
    qa.LoadData();
    qb.LoadData();
  }

  inline void AttachQ() {
    qa.Attach();
    qb.Attach();
  }

  inline void DetachQ() {
    qa.Detach();
    qb.Detach();
  }
};

struct RMTChannelBankSet {
  RMTChannelBank banks[3];
  int current;

  void Setup() {
    for (int i = 0; i < 2; i++) {
      // Detach from RMT for now.
      banks[i].Setup();
      banks[i].DetachQ();
    }
  }
};

// Two identical sets of channel banks are maintained:
// the active one is used for display, when a change is needed it is made to the
// inactive set and swapped out.
static RMTChannelBankSet s_sets[2] = {
    {.banks = {RMTChannelBank(Q1_GPIO, Q2_GPIO), RMTChannelBank(Q5_GPIO, -1),
               RMTChannelBank(Q3_GPIO, Q4_GPIO)},
     .current = 0},
    {.banks = {RMTChannelBank(Q1_GPIO, Q2_GPIO), RMTChannelBank(Q5_GPIO, -1),
               RMTChannelBank(Q3_GPIO, Q4_GPIO)},
     .current = 0},
};
static bool s_switch_set = false;
static int s_active_set = 0;

// This is especially time critical: we must kick off all channels as close to
// simultaneously as possible.
IRAM void StartRMT() {
  uint32_t sv1 = s_sets[0].banks[0].rclk.conf1_start;
  uint32_t sv2 = s_sets[0].banks[0].r.conf1_start;
  uint32_t rmt_reg_base = RMT_CH0CONF1_REG;
  __asm__ __volatile__(
      "rsil a8, 15\n"
      "s32i.n  %[sv1], %[rrb], 0*8\n"
      "s32i.n  %[sv1], %[rrb], 1*8\n"
      "s32i.n  %[sv1], %[rrb], 2*8\n"
      "s32i.n  %[sv2], %[rrb], 3*8\n"
      "s32i.n  %[sv2], %[rrb], 4*8\n"
      "s32i.n  %[sv2], %[rrb], 5*8\n"
      "s32i.n  %[sv1], %[rrb], 6*8\n"
      "s32i.n  %[sv1], %[rrb], 7*8\n"
      "wsr.ps  a8\n"
      : /* out */
      : /* in */[ rrb ] "a"(rmt_reg_base), [ sv1 ] "a"(sv1), [ sv2 ] "a"(sv2)
      : /* temp */ "a8", "memory");
}

uint16_t rl = 100, gl = 100, bl = 70, dl = 100;

// Advance to the next bank in the active set.
IRAM void RMTIntHandler(void *arg) {
  RMTChannelBankSet *abs = &s_sets[s_active_set];
  RMTChannelBank *old_bank = &abs->banks[abs->current];
  abs->current++;
  if (abs->current == ARRAY_SIZE(abs->banks)) {
    if (s_switch_set) {
      s_active_set ^= 1;
      abs = &s_sets[s_active_set];
      abs->current = 0;
      s_switch_set = false;
    } else {
      abs->current = 0;
    }
  }
  RMTChannelBank *new_bank = &abs->banks[abs->current];
  new_bank->LoadData();
  old_bank->DetachQ();
  StartRMT();
  new_bank->AttachQ();
  RMT.int_clr.val = RMT_CH0_TX_END_INT_CLR;
  mgos_gpio_toggle(16);
  (void) arg;
}

IRAM void SendDigits(uint8_t digits[5]) {
  static bool s_started = false;

  s_switch_set = false;
  int inactive_set = (s_active_set ^ 1);
  RMTChannelBankSet *ibs = &s_sets[inactive_set];
  RMTChannelBank *ib0 = &ibs->banks[0];
  ib0->ClearData();
  ib0->GenDigitA(digits[0], rl, gl, bl);
  ib0->GenDigitB(digits[1], rl, gl, bl);
  RMTChannelBank *ib1 = &ibs->banks[1];
  ib1->ClearData();
  ib1->GenDigitA(digits[2], rl, gl, bl);
  RMTChannelBank *ib2 = &ibs->banks[2];
  ib2->ClearData();
  ib2->GenDigitA(digits[3], rl, gl, bl);
  ib2->GenDigitB(digits[4], rl, gl, bl);

  if (dl > 0) {
    ib2->GenIdleSeq(dl);
  }

  if (!s_started) {
    RMTChannelBank *ab0 = &s_sets[inactive_set].banks[0];
    ab0->LoadData();
    s_active_set = inactive_set;
    RMT.int_clr.val = RMT_CH0_TX_END_INT_CLR;
    RMT.int_ena.val = RMT_CH0_TX_END_INT_CLR;
    StartRMT();
    ab0->AttachQ();
    s_started = true;
  } else {
    s_switch_set = true;
  }
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
  char time_str[9];
  mgos_strftime(time_str, sizeof(time_str), "%H:%M:%S", (int) mg_time());
  uint8_t s2 = (time_str[7] % 2 == 0 ? 0xff : 0b10101111);
  uint8_t digits[5] = {s_syms[time_str[0] - '0'], s_syms[time_str[1] - '0'], s2,
                       s_syms[time_str[3] - '0'], s_syms[time_str[4] - '0']};
  SendDigits(digits);
  LOG(LL_INFO, ("%s", time_str));
  (void) arg;
}

static mgos::ScopedTimer s_tmr(std::bind(TimerCB, nullptr));

bool InitApp() {
  periph_module_enable(PERIPH_RMT_MODULE);
  RMT.apb_conf.fifo_mask = 1;
  RMT.apb_conf.mem_tx_wrap_en = 0;
  for (int i = 0; i < 2; i++) {
    s_sets[i].Setup();
  }

  mgos_gpio_setup_output(16, 0);

  intr_handle_t inth;
  esp_intr_alloc(ETS_RMT_INTR_SOURCE, 0, RMTIntHandler, nullptr, &inth);
  esp_intr_set_in_iram(inth, true);

  mg_rpc_add_handler(mgos_rpc_get_global(), "Clock.SetColor",
                     "{r: %d, g: %d, b: %d, d: %d}", SetColorHandler, nullptr);

  s_tmr.Reset(1000, MGOS_TIMER_REPEAT);
  return true;
}

}  // namespace clk

extern "C" {
enum mgos_app_init_result mgos_app_init(void) {
  return (clk::InitApp() ? MGOS_APP_INIT_SUCCESS : MGOS_APP_INIT_ERROR);
}
}
