// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer2_ctc, Timer/Counter2 of the ATmega328P in CTC.
 *
 * It breaks analogWrite() on pins 3 and 11, and tone(), which use Timer2 as
 * well. Channel A toggles at the top, OCR2A; channel B at 0.
 * A sketch uses timer2_ctc or timer2, not both: with both it fails to link.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_CTC_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_CTC_H_

#include "atmega_timer/ctc.h"
#include "atmega_timer/timer2_traits.h"

namespace atmega_timer {

/*!
 * \brief The type of timer2_ctc, for its constants: `Timer2Ctc::kPeriodMax`.
 */
using Timer2Ctc = Ctc<Timer2Traits>;

/*!
 * \brief Timer/Counter2 in CTC; its compare match A interrupt, at the top,
 *        calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer2Ctc timer2_ctc;

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_CTC_H_
