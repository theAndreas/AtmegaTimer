// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer5_ctc, Timer/Counter5 of the ATmega2560 in CTC.
 *
 * Only on the ATmega2560: on another board, this header declares nothing. It
 * breaks analogWrite() on pins 46, 45 and 44, and `Servo`, as Timer5 is
 * the first of the timers `Servo` takes on the Mega. All three channels toggle:
 * ICR5 is the top. A sketch uses timer5_ctc or timer5, not both: with both it
 * fails to link.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_CTC_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_CTC_H_

#ifdef __AVR_ATmega2560__

#include "atmega_timer/ctc.h"
#include "atmega_timer/timer5_traits.h"

namespace atmega_timer {

/*!
 * \brief The type of timer5_ctc, for its constants: `Timer5Ctc::kPeriodMax`.
 */
using Timer5Ctc = Ctc<Timer5Traits>;

/*!
 * \brief Timer/Counter5 in CTC; its input capture interrupt, which mode 12
 *        raises at the top, calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer5Ctc timer5_ctc;

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_CTC_H_
