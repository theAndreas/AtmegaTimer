// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Gives a square wave of 500 Hz on pin 3 by the toggle of channel B of Timer2
// in CTC, without the CPU, and toggles the LED from the callback every half
// second.

#include <AtmegaTimer.h>
#include <stdint.h>

namespace {

constexpr uint8_t kLedPin{LED_BUILTIN};
// The period in microseconds: the toggle is high for one and low for the next.
constexpr uint16_t kPeriod{1000};
// The periods between two toggles of the LED.
constexpr uint16_t kPeriodsPerBlink{500};

void OnMatch() {
  // Only the interrupt reads and writes it.
  static uint16_t periods{0};
  ++periods;
  if (periods < kPeriodsPerBlink) {
    return;
  }
  periods = 0;
  digitalWrite(kLedPin, digitalRead(kLedPin) == LOW ? HIGH : LOW);
}

}  // namespace

void setup() {
  pinMode(kLedPin, OUTPUT);
  // Nothing to do on a refusal here: the period is in range, the timer is
  // begun once and before the toggle.
  atmega_timer::Discard(atmega_timer::timer2_ctc.Begin(kPeriod, OnMatch));
  atmega_timer::Discard(
      atmega_timer::timer2_ctc.EnableToggle<atmega_timer::Channel::kB>());
  atmega_timer::Discard(atmega_timer::timer2_ctc.Start());
}

void loop() {}
