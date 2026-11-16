// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/timer3.h"

#ifdef __AVR_ATmega2560__

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/timer3_traits.h"

namespace atmega_timer {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Timer3 timer3;

// timer3_ctc.cpp defines it as well, so that a sketch takes one of the two.
uint8_t const internal::kOneObjectOnTimer3{0u};

}  // namespace atmega_timer

ISR(TIMER3_OVF_vect) { atmega_timer::timer3.OnInterrupt(); }

#endif  // __AVR_ATmega2560__
