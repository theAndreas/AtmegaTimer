// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer4_ctc, Timer/Counter4 of the ATmega2560 in CTC.
 *
 * Only on the ATmega2560: on another board, this header declares nothing. It
 * breaks analogWrite() on pins 6, 7 and 8, and `Servo`, as Timer4 is
 * the last of the timers `Servo` takes on the Mega, after Timer5,
 * Timer1 and Timer3. All three channels toggle: ICR4 is the top.
 * A sketch uses timer4_ctc or timer4, not both: with both it fails to link.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_CTC_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_CTC_H_

#ifdef __AVR_ATmega2560__

#include "atmega_timer/ctc.h"
#include "atmega_timer/timer4_traits.h"

namespace atmega_timer {

/*!
 * \brief The type of timer4_ctc, for its constants: `Timer4Ctc::kPeriodMax`.
 */
using Timer4Ctc = Ctc<Timer4Traits>;

/*!
 * \brief Timer/Counter4 in CTC; its input capture interrupt, which mode 12
 *        raises at the top, calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer4Ctc timer4_ctc;

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_CTC_H_
