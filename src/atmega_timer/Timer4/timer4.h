// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer4, Timer/Counter4 of the ATmega2560 as a Timer.
 *
 * Only on the ATmega2560: on another board, this header declares nothing. It
 * breaks analogWrite() on pins 6, 7 and 8, and `Servo`, as Timer4 is
 * the last of the timers `Servo` takes on the Mega, after Timer5,
 * Timer1 and Timer3. All three channels give a PWM of their own: ICR4 is the
 * top.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_TIMER4_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_TIMER4_H_

#ifdef __AVR_ATmega2560__

#include "atmega_timer/Modes/phase_correct_pwm.h"
#include "atmega_timer/Timer4/timer4_traits.h"

namespace atmega_timer {

/*! \brief The type of timer4, for its constants: `Timer4::kPeriodMax`. */
using Timer4 = PhaseCorrectPwm<Timer4Traits>;

/*!
 * \brief Timer/Counter4; its overflow interrupt calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer4 timer4;

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_TIMER4_H_
