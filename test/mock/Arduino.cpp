// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Arduino.h"

#include <stddef.h>

#include <string>

MockSerial Serial;

size_t Print::print(__FlashStringHelper const* text) {
  return print(reinterpret_cast<char const*>(text));
}

size_t Print::print(char const* text) {
  size_t count{0};
  while (*text != '\0') {
    count += write(static_cast<uint8_t>(*text++));
  }
  return count;
}

size_t Print::print(unsigned int number) {
  return print(std::to_string(number).c_str());
}

size_t Print::println() { return print("\r\n"); }
