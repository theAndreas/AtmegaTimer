// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The part of the Arduino core that AtmegaTimer uses, for the tests on the
// host: pinMode(), which records the mode of each pin for the tests to read.
// The core's constants are macros; here they are constants by the same names,
// which is all the library needs. Extend it as the library comes to use more
// of the core.

#ifndef ATMEGATIMER_TEST_MOCK_ARDUINO_H_
#define ATMEGATIMER_TEST_MOCK_ARDUINO_H_

#include <stdint.h>

#include <array>

// The core's names.
// NOLINTBEGIN(readability-identifier-naming)
constexpr uint8_t INPUT{0x0};
constexpr uint8_t OUTPUT{0x1};

// The core's function, which records the mode.
void pinMode(uint8_t pin, uint8_t mode);
// NOLINTEND(readability-identifier-naming)

// The mode pinMode() last set for each of the Uno's 20 pins, INPUT after a
// reset.
extern std::array<uint8_t, 20> mock_pin_mode;

#endif  // ATMEGATIMER_TEST_MOCK_ARDUINO_H_
