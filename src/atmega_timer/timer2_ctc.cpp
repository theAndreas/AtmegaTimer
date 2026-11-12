// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/timer2_ctc.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/timer2_traits.h"

namespace atmega_timer {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Timer2Ctc timer2_ctc;

// timer2.cpp defines it as well, so that a sketch takes one of the two.
uint8_t const internal::kOneObjectOnTimer2{0U};

}  // namespace atmega_timer

ISR(TIMER2_COMPA_vect) { atmega_timer::timer2_ctc.OnInterrupt(); }
