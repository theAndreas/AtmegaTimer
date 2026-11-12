// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/timer1.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/timer1_traits.h"

namespace atmega_timer {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Timer1 timer1;

// timer1_ctc.cpp defines it as well, so that a sketch takes one of the two.
uint8_t const internal::kOneObjectOnTimer1{0U};

}  // namespace atmega_timer

ISR(TIMER1_OVF_vect) { atmega_timer::timer1.OnInterrupt(); }
