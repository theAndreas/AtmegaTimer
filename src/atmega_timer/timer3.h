// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer3, Timer/Counter3 of the ATmega2560 as a Timer.
 *
 * Only on the ATmega2560: on another board, this header declares nothing. It
 * breaks analogWrite() on pins 5, 2 and 3, and `Servo`, as Timer3 is
 * the third of the timers `Servo` takes on the Mega, after Timer5 and
 * Timer1. All three channels give a PWM of their own: ICR3 is the top.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_H_

#ifdef __AVR_ATmega2560__

#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/timer3_traits.h"

namespace atmega_timer {

/*! \brief The type of timer3, for its constants: `Timer3::kPeriodMax`. */
using Timer3 = PhaseCorrectPwm<Timer3Traits>;

/*!
 * \brief Timer/Counter3; its overflow interrupt calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer3 timer3;

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_H_
