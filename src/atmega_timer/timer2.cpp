// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/timer2.h"

#include <avr/interrupt.h>
#include <avr/io.h>

namespace atmega_timer {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Timer2 timer2;

}  // namespace atmega_timer

ISR(TIMER2_OVF_vect) { atmega_timer::timer2.OnOverflow(); }
