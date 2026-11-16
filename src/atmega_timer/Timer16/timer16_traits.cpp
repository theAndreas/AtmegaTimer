// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/Timer16/timer16_traits.h"

#include <stdint.h>

namespace atmega_timer {

namespace {

// The prescalers 8 to 256 as shifts; 1024 is kTimer16ShiftLargest.
constexpr uint8_t kShiftDivide8{3u};
constexpr uint8_t kShiftDivide64{6u};
constexpr uint8_t kShiftDivide256{8u};

}  // namespace

// A switch rather than a table, which avr-gcc would copy to RAM.
uint8_t internal::Timer16ShiftOf(Timer16ClockSelect clock_select) {
  switch (clock_select) {
    case Timer16ClockSelect::kDivide8:
      return kShiftDivide8;
    case Timer16ClockSelect::kDivide64:
      return kShiftDivide64;
    case Timer16ClockSelect::kDivide256:
      return kShiftDivide256;
    case Timer16ClockSelect::kDivide1024:
      return kTimer16ShiftLargest;
    case Timer16ClockSelect::kNoClock:
    case Timer16ClockSelect::kDivide1:
    default:
      return 0u;
  }
}

}  // namespace atmega_timer
