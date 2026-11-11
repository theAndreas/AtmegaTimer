// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Two PWMs of their own duty from Timer1, a quarter on pin 9 and three
// quarters on pin 10, at 1 kHz; after a second at 500 Hz. Timer1 takes the
// new period at the next bottom of its counter, so that no PWM loses a pulse,
// and the time it reads until then is still in the old period. Each call that
// can refuse is checked, and a refusal printed with its reason.

#include <AtmegaTimer.h>
#include <stdint.h>

namespace {

using atmega_timer::Channel;
using TimeType = atmega_timer::Timer1::TimeType;

constexpr uint32_t kBaudRate{115200};
constexpr TimeType kFastPeriod{1000};
constexpr TimeType kSlowPeriod{2000};
// Shares of atmega_timer::kDutyMax, 1023.
constexpr uint16_t kQuarter{256};
constexpr uint16_t kThreeQuarters{767};

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

}  // namespace

void setup() {
  Serial.begin(kBaudRate);
  auto& timer = atmega_timer::timer1;
  Check(timer.Begin(kFastPeriod), F("Begin"));
  Check(timer.EnablePwm<Channel::kA>(kQuarter), F("EnablePwm A"));
  Check(timer.EnablePwm<Channel::kB>(kThreeQuarters), F("EnablePwm B"));
  Check(timer.Start(), F("Start"));

  delay(1000);
  Check(timer.SetPeriod(kSlowPeriod), F("SetPeriod"));
  atmega_timer::Result<TimeType> const elapsed{timer.Read()};
  Check(elapsed.GetError(), F("Read"));
  Serial.print(F("Since the bottom: "));
  Serial.print(elapsed.GetValue());
  Serial.println(F(" us"));
}

void loop() {}
