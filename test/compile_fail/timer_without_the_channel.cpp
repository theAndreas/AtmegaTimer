// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Timer2 has channel B only, as OCR2A is its top: a PWM on channel A does not
// compile.
// Expected: the timer has no such channel

// The clock a board's build defines, which this call of the compiler does not.
#define F_CPU 16000000UL

#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/status.h"
#include "atmega_timer/timer2.h"

void EnablePwmOnChannelA() {
  atmega_timer::Discard(
      atmega_timer::timer2.EnablePwm<atmega_timer::Channel::kA>(512));
}
