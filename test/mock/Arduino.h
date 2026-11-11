// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The part of the Arduino core that the library uses, for the tests on the
// host: __FlashStringHelper and F(), and Print and Serial, which print into a
// std::string the tests read. The host has no separate flash, so a text "in
// flash" is a text in RAM here; that the library reads flash with pgm_read is
// what SelfCheck in simavr checks, not these tests. Extend it as the library
// comes to use more of the core.

#ifndef ATMEGATIMER_TEST_MOCK_ARDUINO_H_
#define ATMEGATIMER_TEST_MOCK_ARDUINO_H_

#include <stddef.h>
#include <stdint.h>

#include <string>

// The core's name, reserved as it is: the library takes this very type.
class __FlashStringHelper;  // NOLINT(bugprone-reserved-identifier)

// The core's macros, as it defines them.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PSTR(text) (text)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define F(text) (reinterpret_cast<__FlashStringHelper const*>(PSTR(text)))

// As the core's Print: write() is what a derived class provides, print()
// what the library calls. Copied and moved as the core's is, by the compiler.
class Print {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  virtual ~Print() = default;

  virtual size_t write(uint8_t c) = 0;

  size_t print(__FlashStringHelper const* text);
  size_t print(char const* text);
  size_t print(char c) { return write(static_cast<uint8_t>(c)); }
  size_t print(unsigned int number);
  size_t println();
};

// What the sketch would print on Serial, in `output`.
class MockSerial : public Print {
 public:
  size_t write(uint8_t c) override {
    output.push_back(static_cast<char>(c));
    return 1;
  }

  std::string output;
};

// The core's name for it; a global, as test/.clang-tidy allows.
extern MockSerial Serial;  // NOLINT(readability-identifier-naming)

#endif  // ATMEGATIMER_TEST_MOCK_ARDUINO_H_
