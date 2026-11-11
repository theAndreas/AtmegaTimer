// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// How a sketch uses AtmegaTimer: a reading of a pin, limited to a range,
// on Serial.

#include <AtmegaTimer.h>

void setup() { Serial.begin(115200); }

void loop() {
  // From a pin, so that the compiler cannot fold the call away.
  int const reading{analogRead(A0)};
  Serial.println(atmega_timer::Clamp(reading, 100, 900));
  delay(1000);
}
