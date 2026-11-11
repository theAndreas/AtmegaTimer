// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Checks the library on the board, or in simavr through tools/self_check.sh,
// and prints one line per check and the number of checks that failed.
// SelfCheck.expected is what a correct run prints; a variant that prints
// otherwise has a SelfCheck.<variant>.expected.

#include <AtmegaTimer.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

using atmega_timer::Clamp;

namespace {

int failed{0};

// Prints "pass" or "FAIL" and what was checked, and counts a failure.
void Check(bool passed, __FlashStringHelper const* what) {
  if (!passed) {
    ++failed;
  }
  Serial.print(passed ? F("pass ") : F("FAIL "));
  Serial.println(what);
}

}  // namespace

void setup() {
  Serial.begin(115200);

  // volatile, so that the checks run on the board rather than in the compiler.
  int const volatile value{42};
  Check(Clamp(value, 0, 10) == 10, F("Clamp limits a value above"));
  Check(Clamp(-value, 0, 10) == 0, F("Clamp limits a value below"));
  Check(Clamp(value, 0, 100) == 42, F("Clamp keeps a value in range"));

  Serial.print(F("failed: "));
  Serial.println(failed);
  Serial.flush();

  // Sleeping with interrupts off ends simavr; on a board it just stops.
  cli();
  sleep_cpu();
}

void loop() {}
