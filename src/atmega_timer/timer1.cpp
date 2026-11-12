// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/timer1.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

namespace atmega_timer {

namespace {

// The prescalers 8 to 256 as shifts; 1024 is kShiftLargest.
constexpr uint8_t kShiftDivide8{3U};
constexpr uint8_t kShiftDivide64{6U};
constexpr uint8_t kShiftDivide256{8U};

}  // namespace

// A switch rather than a table, which avr-gcc would copy to RAM.
uint8_t Timer1Traits::ShiftOf(ClockSelect clock_select) {
  switch (clock_select) {
    case ClockSelect::kDivide8:
      return kShiftDivide8;
    case ClockSelect::kDivide64:
      return kShiftDivide64;
    case ClockSelect::kDivide256:
      return kShiftDivide256;
    case ClockSelect::kDivide1024:
      return kShiftLargest;
    case ClockSelect::kNoClock:
    case ClockSelect::kDivide1:
    default:
      return 0U;
  }
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Timer1 timer1;

}  // namespace atmega_timer

ISR(TIMER1_OVF_vect) { atmega_timer::timer1.OnInterrupt(); }
