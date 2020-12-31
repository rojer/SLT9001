/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 */

#include "clk_br_curve.hpp"

namespace clk {

const BrightnessCurveEntry s_brightness_curve[101] = {
    {.pct = 100, .dl = 0, .sf = 1.0},    // 450 mA
    {.pct = 99, .dl = 3, .sf = 1.0},     // 445 mA
    {.pct = 98, .dl = 6, .sf = 1.0},     // 441 mA
    {.pct = 97, .dl = 9, .sf = 1.0},     // 436 mA
    {.pct = 96, .dl = 12, .sf = 1.0},    // 432 mA
    {.pct = 95, .dl = 15, .sf = 1.0},    // 427 mA
    {.pct = 94, .dl = 18, .sf = 1.0},    // 423 mA
    {.pct = 93, .dl = 21, .sf = 1.0},    // 418 mA
    {.pct = 92, .dl = 24, .sf = 1.0},    // 414 mA
    {.pct = 91, .dl = 26, .sf = 1.0},    // 410 mA
    {.pct = 90, .dl = 30, .sf = 1.0},    // 405 mA
    {.pct = 89, .dl = 33, .sf = 1.0},    // 401 mA
    {.pct = 88, .dl = 37, .sf = 1.0},    // 396 mA
    {.pct = 87, .dl = 40, .sf = 1.0},    // 392 mA
    {.pct = 86, .dl = 44, .sf = 1.0},    // 387 mA
    {.pct = 85, .dl = 47, .sf = 1.0},    // 383 mA
    {.pct = 84, .dl = 52, .sf = 1.0},    // 378 mA
    {.pct = 83, .dl = 56, .sf = 1.0},    // 374 mA
    {.pct = 82, .dl = 59, .sf = 1.0},    // 370 mA
    {.pct = 81, .dl = 63, .sf = 1.0},    // 365 mA
    {.pct = 80, .dl = 67, .sf = 1.0},    // 361 mA
    {.pct = 79, .dl = 72, .sf = 1.0},    // 356 mA
    {.pct = 78, .dl = 76, .sf = 1.0},    // 352 mA
    {.pct = 77, .dl = 81, .sf = 1.0},    // 347 mA
    {.pct = 76, .dl = 85, .sf = 1.0},    // 343 mA
    {.pct = 75, .dl = 90, .sf = 1.0},    // 338 mA
    {.pct = 74, .dl = 94, .sf = 1.0},    // 334 mA
    {.pct = 73, .dl = 99, .sf = 1.0},    // 330 mA
    {.pct = 72, .dl = 104, .sf = 1.0},   // 325 mA
    {.pct = 71, .dl = 109, .sf = 1.0},   // 321 mA
    {.pct = 70, .dl = 116, .sf = 1.0},   // 316 mA
    {.pct = 69, .dl = 119, .sf = 1.0},   // 312 mA
    {.pct = 68, .dl = 125, .sf = 1.0},   // 307 mA
    {.pct = 67, .dl = 130, .sf = 1.0},   // 303 mA
    {.pct = 66, .dl = 137, .sf = 1.0},   // 298 mA
    {.pct = 65, .dl = 143, .sf = 1.0},   // 294 mA
    {.pct = 64, .dl = 149, .sf = 1.0},   // 290 mA
    {.pct = 63, .dl = 155, .sf = 1.0},   // 285 mA
    {.pct = 62, .dl = 161, .sf = 1.0},   // 281 mA
    {.pct = 61, .dl = 170, .sf = 1.0},   // 276 mA
    {.pct = 60, .dl = 175, .sf = 1.0},   // 272 mA
    {.pct = 59, .dl = 185, .sf = 1.0},   // 267 mA
    {.pct = 58, .dl = 191, .sf = 1.0},   // 263 mA
    {.pct = 57, .dl = 199, .sf = 1.0},   // 258 mA
    {.pct = 56, .dl = 205, .sf = 1.0},   // 254 mA
    {.pct = 55, .dl = 213, .sf = 1.0},   // 250 mA
    {.pct = 54, .dl = 222, .sf = 1.0},   // 245 mA
    {.pct = 53, .dl = 231, .sf = 1.0},   // 241 mA
    {.pct = 52, .dl = 242, .sf = 1.0},   // 236 mA
    {.pct = 51, .dl = 250, .sf = 1.0},   // 232 mA
    {.pct = 50, .dl = 261, .sf = 1.0},   // 227 mA
    {.pct = 49, .dl = 271, .sf = 1.0},   // 223 mA
    {.pct = 48, .dl = 283, .sf = 1.0},   // 218 mA
    {.pct = 47, .dl = 293, .sf = 1.0},   // 214 mA
    {.pct = 46, .dl = 305, .sf = 1.0},   // 210 mA
    {.pct = 45, .dl = 317, .sf = 1.0},   // 205 mA
    {.pct = 44, .dl = 329, .sf = 1.0},   // 201 mA
    {.pct = 43, .dl = 344, .sf = 1.0},   // 196 mA
    {.pct = 42, .dl = 356, .sf = 1.0},   // 192 mA
    {.pct = 41, .dl = 374, .sf = 1.0},   // 187 mA
    {.pct = 40, .dl = 386, .sf = 1.0},   // 183 mA
    {.pct = 39, .dl = 403, .sf = 1.0},   // 178 mA
    {.pct = 38, .dl = 420, .sf = 1.0},   // 174 mA
    {.pct = 37, .dl = 435, .sf = 1.0},   // 170 mA
    {.pct = 36, .dl = 457, .sf = 1.0},   // 165 mA
    {.pct = 35, .dl = 475, .sf = 1.0},   // 161 mA
    {.pct = 34, .dl = 498, .sf = 1.0},   // 156 mA
    {.pct = 33, .dl = 516, .sf = 1.0},   // 152 mA
    {.pct = 32, .dl = 544, .sf = 1.0},   // 147 mA
    {.pct = 31, .dl = 567, .sf = 1.0},   // 143 mA
    {.pct = 30, .dl = 598, .sf = 1.0},   // 138 mA
    {.pct = 29, .dl = 622, .sf = 1.0},   // 134 mA
    {.pct = 28, .dl = 652, .sf = 1.0},   // 130 mA
    {.pct = 27, .dl = 680, .sf = 1.0},   // 125 mA
    {.pct = 26, .dl = 712, .sf = 1.0},   // 121 mA
    {.pct = 25, .dl = 755, .sf = 1.0},   // 116 mA
    {.pct = 24, .dl = 790, .sf = 1.0},   // 112 mA
    {.pct = 23, .dl = 846, .sf = 1.0},   // 107 mA
    {.pct = 22, .dl = 875, .sf = 1.0},   // 103 mA
    {.pct = 21, .dl = 937, .sf = 1.0},   // 98 mA
    {.pct = 20, .dl = 987, .sf = 1.0},   // 94 mA
    {.pct = 19, .dl = 1040, .sf = 1.0},  // 90 mA
    {.pct = 18, .dl = 1120, .sf = 1.0},  // 85 mA
    {.pct = 17, .dl = 1200, .sf = 1.0},  // 81 mA
    {.pct = 16, .dl = 1275, .sf = 1.0},  // 76 mA
    {.pct = 15, .dl = 1350, .sf = 1.0},  // 72 mA
    {.pct = 14, .dl = 1465, .sf = 1.0},  // 67 mA
    {.pct = 13, .dl = 1575, .sf = 1.0},  // 63 mA
    {.pct = 12, .dl = 1735, .sf = 1.0},  // 58 mA
    {.pct = 11, .dl = 1900, .sf = 1.0},  // 54 mA
    {.pct = 10, .dl = 2000, .sf = 1.0},  // 50 mA
    {.pct = 9, .dl = 2000, .sf = 0.88},  // 45 mA; 175
    {.pct = 8, .dl = 2000, .sf = 0.77},  // 40 mA; 153
    {.pct = 7, .dl = 2000, .sf = 0.66},  // 35 mA; 132
    {.pct = 6, .dl = 2000, .sf = 0.55},  // 30 mA; 109
    {.pct = 5, .dl = 2000, .sf = 0.47},  // 25 mA; 93
    {.pct = 4, .dl = 2000, .sf = 0.38},  // 20 mA; 75
    {.pct = 3, .dl = 2000, .sf = 0.28},  // 15 mA; 56
    {.pct = 2, .dl = 2000, .sf = 0.19},  // 10 mA; 38
    {.pct = 1, .dl = 2000, .sf = 0.10},  // 5 mA; 20
    {.pct = 0, .dl = 0, .sf = 0},        // 0 mA
};

BrightnessCurveEntry GetBrightnessCurveEntry(int br_pct) {
  if (br_pct > 100) br_pct = 100;
  if (br_pct < 0) br_pct = 0;
  return s_brightness_curve[100 - br_pct];
}

}  // namespace clk
