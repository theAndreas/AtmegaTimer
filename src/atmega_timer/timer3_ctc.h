// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer3_ctc, Timer/Counter3 of the ATmega2560 in CTC.
 *
 * Only on the ATmega2560: on another board, this header declares nothing. It
 * breaks analogWrite() on pins 5, 2 and 3, and `Servo`, as Timer3 is
 * the third of the timers `Servo` takes on the Mega, after Timer5 and
 * Timer1. All three channels toggle: ICR3 is the top.
 * A sketch uses timer3_ctc or timer3, not both: with both it fails to link.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_CTC_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_CTC_H_

#ifdef __AVR_ATmega2560__

#include "atmega_timer/ctc.h"
#include "atmega_timer/timer3_traits.h"

namespace atmega_timer {

/*!
 * \brief The type of timer3_ctc, for its constants: `Timer3Ctc::kPeriodMax`.
 */
using Timer3Ctc = Ctc<Timer3Traits>;

/*!
 * \brief Timer/Counter3 in CTC; its input capture interrupt, which mode 12
 *        raises at the top, calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer3Ctc timer3_ctc;

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_CTC_H_
