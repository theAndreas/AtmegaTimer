// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/Timer2/timer2_traits.h"

#include <stdint.h>

namespace atmega_timer {

namespace {

// The prescalers 8 to 256 as shifts; 1024 is kShiftLargest.
constexpr uint8_t kShiftDivide8{3u};
constexpr uint8_t kShiftDivide32{5u};
constexpr uint8_t kShiftDivide64{6u};
constexpr uint8_t kShiftDivide128{7u};
constexpr uint8_t kShiftDivide256{8u};

}  // namespace

// A switch rather than a table, which avr-gcc would copy to RAM.
uint8_t Timer2Traits::ShiftOf(ClockSelect clock_select) {
  switch (clock_select) {
    case ClockSelect::kDivide8:
      return kShiftDivide8;
    case ClockSelect::kDivide32:
      return kShiftDivide32;
    case ClockSelect::kDivide64:
      return kShiftDivide64;
    case ClockSelect::kDivide128:
      return kShiftDivide128;
    case ClockSelect::kDivide256:
      return kShiftDivide256;
    case ClockSelect::kDivide1024:
      return kShiftLargest;
    case ClockSelect::kNoClock:
    case ClockSelect::kDivide1:
    default:
      return 0u;
  }
}

}  // namespace atmega_timer
