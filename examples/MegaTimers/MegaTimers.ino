// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The three timers the Mega has beyond the Uno, each in its own way: three
// PWMs of their own duty from Timer3, a quarter on pin 5, a half on pin 2 and
// three quarters on pin 3, at 1 kHz; a square wave of 500 Hz on pin 8 by the
// toggle of channel C of Timer4 in CTC; and the LED toggled every half second
// from the interrupt of Timer5. Each call that can refuse is checked, and a
// refusal printed with its reason. For the Mega alone: on another board it does
// not compile, as Timer3 to Timer5 are the ATmega2560's.

#include <AtmegaTimer.h>
#include <stdint.h>

#ifndef __AVR_ATmega2560__
#error \
    "This example needs the ATmega2560 of the Mega, which has Timer3 to Timer5"
#endif

namespace {

using atmega_timer::Channel;

constexpr uint8_t kLedPin{LED_BUILTIN};
constexpr uint32_t kBaudRate{115200};
// The periods in microseconds: of the PWMs, of the toggle, which is high for
// one period and low for the next, and of the LED.
constexpr uint32_t kPwmPeriod{1000};
constexpr uint32_t kTogglePeriod{1000};
constexpr uint32_t kLedPeriod{500000};
// Shares of atmega_timer::kDutyMax, 1023.
constexpr uint16_t kQuarter{256};
constexpr uint16_t kHalf{512};
constexpr uint16_t kThreeQuarters{767};

void ToggleLed() {
  digitalWrite(kLedPin, digitalRead(kLedPin) == LOW ? HIGH : LOW);
}

// Prints why a call was refused and stops there, as the rest would not work.
void Check(atmega_timer::Status status, __FlashStringHelper const* call) {
  if (!status.IsOk()) {
    Serial.print(call);
    Serial.print(F(": "));
    Serial.println(status.GetErrorText());
    while (true) {
    }
  }
}

void StartPwms() {
  auto& timer{atmega_timer::timer3};
  Check(timer.Begin(kPwmPeriod), F("timer3 Begin"));
  Check(timer.EnablePwm<Channel::kA>(kQuarter), F("timer3 EnablePwm A"));
  Check(timer.EnablePwm<Channel::kB>(kHalf), F("timer3 EnablePwm B"));
  Check(timer.EnablePwm<Channel::kC>(kThreeQuarters), F("timer3 EnablePwm C"));
  Check(timer.Start(), F("timer3 Start"));
}

void StartToggle() {
  auto& timer{atmega_timer::timer4_ctc};
  Check(timer.Begin(kTogglePeriod), F("timer4_ctc Begin"));
  Check(timer.EnableToggle<Channel::kC>(), F("timer4_ctc EnableToggle C"));
  Check(timer.Start(), F("timer4_ctc Start"));
}

void StartLed() {
  auto& timer{atmega_timer::timer5};
  Check(timer.Begin(kLedPeriod, ToggleLed), F("timer5 Begin"));
  Check(timer.Start(), F("timer5 Start"));
}

}  // namespace

void setup() {
  Serial.begin(kBaudRate);
  pinMode(kLedPin, OUTPUT);
  StartPwms();
  StartToggle();
  StartLed();
}

void loop() {}
