// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Min(), Max() and Clamp(), a value limited to a range.
 *
 * The starting point the template wrote: replace it with the library.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_CLAMP_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_CLAMP_H_

#include "atmega_timer/attributes.h"

/*! \brief Periodic interrupt, PWM and time readout on Timer1 to Timer5 of the ATmega328P and ATmega2560. */
namespace atmega_timer {

/*!
 * \brief Returns the lesser of `a` and `b`, `a` if they are equal.
 *
 * A function, unlike the macro `min` of the Arduino core, which evaluates an
 * argument twice.
 *
 * \tparam Value A type with operator<, such as an integer.
 * \param[in] a The one value.
 * \param[in] b The other.
 * \return The lesser.
 */
template <typename Value>
ATMEGATIMER_NODISCARD constexpr Value Min(Value a, Value b) {
  return b < a ? b : a;
}

/*!
 * \brief Returns the greater of `a` and `b`, `a` if they are equal.
 *
 * \tparam Value A type with operator<, such as an integer.
 * \param[in] a The one value.
 * \param[in] b The other.
 * \return The greater.
 */
template <typename Value>
ATMEGATIMER_NODISCARD constexpr Value Max(Value a, Value b) {
  return a < b ? b : a;
}

/*!
 * \brief Returns `value`, limited to the range from `low` to `high`.
 *
 * \tparam Value A type with operator<, such as an integer.
 * \param[in] value The value to limit.
 * \param[in] low The least value returned.
 * \param[in] high The greatest value returned.
 * \return `low` if `value` lies below it, `high` if above, else `value`.
 * \pre `low` is not greater than `high`; else the result is `high`.
 */
template <typename Value>
ATMEGATIMER_NODISCARD constexpr Value Clamp(Value value, Value low, Value high) {
  // One return, as a constexpr function of C++11 allows no more.
  return Min(Max(value, low), high);
}

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_CLAMP_H_
