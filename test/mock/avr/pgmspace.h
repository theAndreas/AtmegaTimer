// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// avr-libc's access to flash, for the tests on the host, which has no
// separate flash: PROGMEM is nothing, and a read from flash is a read.

#ifndef ATMEGATIMER_TEST_MOCK_AVR_PGMSPACE_H_
#define ATMEGATIMER_TEST_MOCK_AVR_PGMSPACE_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PROGMEM
#define PGM_P const char*

// avr-libc's names, which the library calls.
// NOLINTNEXTLINE(readability-identifier-naming)
inline uint8_t pgm_read_byte(void const* address) {
  return *static_cast<uint8_t const*>(address);
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline uint16_t pgm_read_word(void const* address) {
  uint16_t value{0};
  // MISRA 24.5.2: reads the bytes of a value at any alignment, the one way
  // without undefined behaviour before C++20's std::bit_cast; sizeof bounds it:
  // NOLINTNEXTLINE(bugprone-unsafe-functions)
  memcpy(&value, address, sizeof(value));
  return value;
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline uint32_t pgm_read_dword(void const* address) {
  uint32_t value{0};
  // NOLINTNEXTLINE(bugprone-unsafe-functions): as pgm_read_word() above
  memcpy(&value, address, sizeof(value));
  return value;
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline void* memcpy_P(void* ram, void const* flash, size_t count) {
  // MISRA 24.5.2: avr-libc's memcpy() from flash, which is RAM on the host;
  // the caller's count bounds it, as it bounds avr-libc's:
  // NOLINTNEXTLINE(bugprone-unsafe-functions)
  return memcpy(ram, flash, count);
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline void const* pgm_read_ptr(void const* address) {
  return *static_cast<void const* const*>(address);
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline int memcmp_P(void const* ram, void const* flash, size_t count) {
  // MISRA 24.5.2: avr-libc's memcmp() with flash, as memcpy_P() above:
  // NOLINTNEXTLINE(bugprone-unsafe-functions)
  return memcmp(ram, flash, count);
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline size_t strlen_P(char const* flash) {
  // MISRA 21.2.2: avr-libc's strlen() in flash, as memcpy_P() above; the
  // caller's text ends in its null, as avr-libc's needs it:
  // NOLINTNEXTLINE(bugprone-unsafe-functions)
  return strlen(flash);
}

#endif  // ATMEGATIMER_TEST_MOCK_AVR_PGMSPACE_H_
