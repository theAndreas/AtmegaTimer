// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer1_ctc, Timer/Counter1 of the ATmega328P in CTC.
 *
 * It breaks `Servo`, which runs on Timer1 on the ATmega328P, and analogWrite()
 * on pins 9 and 10. Both channels toggle: ICR1 is the top.
 * A sketch uses timer1_ctc or timer1, not both: with both it fails to link.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_CTC_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_CTC_H_

#include "atmega_timer/ctc.h"
#include "atmega_timer/timer1_traits.h"

namespace atmega_timer {

/*!
 * \brief The type of timer1_ctc, for its constants: `Timer1Ctc::kPeriodMax`.
 */
using Timer1Ctc = Ctc<Timer1Traits>;

/*!
 * \brief Timer/Counter1 in CTC; its input capture interrupt, which mode 12
 *        raises at the top, calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer1Ctc timer1_ctc;

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_CTC_H_
