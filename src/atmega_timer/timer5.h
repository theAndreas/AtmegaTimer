// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer5, Timer/Counter5 of the ATmega2560 as a Timer.
 *
 * Only on the ATmega2560: on another board, this header declares nothing. It
 * breaks analogWrite() on pins 46, 45 and 44, and `Servo`, as Timer5 is
 * the first of the timers `Servo` takes on the Mega. All three channels give a
 * PWM of their own: ICR5 is the top.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_H_

#ifdef __AVR_ATmega2560__

#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/timer5_traits.h"

namespace atmega_timer {

/*! \brief The type of timer5, for its constants: `Timer5::kPeriodMax`. */
using Timer5 = PhaseCorrectPwm<Timer5Traits>;

/*!
 * \brief Timer/Counter5; its overflow interrupt calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer5 timer5;

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_H_
