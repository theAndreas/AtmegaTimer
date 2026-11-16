// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Arduino pins of the compare outputs of Timer1 and Timer2, as the core's
// pins_arduino.h of the board has them: the Mega's where the tests are built
// for the ATmega2560, the Uno's otherwise. Written down here rather than taken
// from the traits, so that a test shows a pin the traits get wrong.

#ifndef ATMEGATIMER_TEST_PINS_H_
#define ATMEGATIMER_TEST_PINS_H_

#include <stdint.h>

namespace atmega_timer {

#ifdef __AVR_ATmega2560__
constexpr uint8_t kOc1aPin{11};  // PB5
constexpr uint8_t kOc1bPin{12};  // PB6
constexpr uint8_t kOc2aPin{10};  // PB4
constexpr uint8_t kOc2bPin{9};   // PH6
#else
constexpr uint8_t kOc1aPin{9};   // PB1
constexpr uint8_t kOc1bPin{10};  // PB2
constexpr uint8_t kOc2aPin{11};  // PB3
constexpr uint8_t kOc2bPin{3};   // PD3
#endif

}  // namespace atmega_timer

#endif  // ATMEGATIMER_TEST_PINS_H_
