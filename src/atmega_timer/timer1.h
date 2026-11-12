// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer1, Timer/Counter1 of the ATmega328P as a Timer.
 *
 * It breaks `Servo`, which runs on Timer1 on the ATmega328P, and analogWrite()
 * on pins 9 and 10. Both channels give a PWM of their own: ICR1 is the top.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_H_

#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/timer1_traits.h"

namespace atmega_timer {

/*! \brief The type of timer1, for its constants: `Timer1::kPeriodMax`. */
using Timer1 = PhaseCorrectPwm<Timer1Traits>;

/*!
 * \brief Timer/Counter1; its overflow interrupt calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer1 timer1;

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_H_
