// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/Timer5/timer5.h"

#ifdef __AVR_ATmega2560__

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/Timer5/timer5_traits.h"

namespace atmega_timer {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Timer5 timer5;

// timer5_ctc.cpp defines it as well, so that a sketch takes one of the two.
uint8_t const internal::kOneObjectOnTimer5{0u};

}  // namespace atmega_timer

ISR(TIMER5_OVF_vect) { atmega_timer::timer5.OnInterrupt(); }

#endif  // __AVR_ATmega2560__
