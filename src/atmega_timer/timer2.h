// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer2, Timer/Counter2 of the ATmega328P as a Timer.
 *
 * It breaks analogWrite() on pins 3 and 11, and tone(), which use Timer2 as
 * well. Pin 11, OC2A, gives no PWM: OCR2A is the top of the counter.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_H_

#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/timer2_traits.h"

namespace atmega_timer {

/*! \brief The type of timer2, for its constants: `Timer2::kPeriodMax`. */
using Timer2 = PhaseCorrectPwm<Timer2Traits>;

/*!
 * \brief Timer/Counter2; its overflow interrupt calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer2 timer2;

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_H_
