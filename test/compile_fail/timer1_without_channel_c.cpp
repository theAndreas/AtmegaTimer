// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Timer1 has channels A and B: OC1C of the ATmega2560 is left out, as its pin
// joins it with OC0A of Timer0. A PWM on channel C does not compile.
// Expected: the timer has no such channel

// The clock a board's build defines, which this call of the compiler does not.
#define F_CPU 16000000uL

#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/status.h"
#include "atmega_timer/timer1.h"

void EnablePwmOnChannelC() {
  atmega_timer::Discard(
      atmega_timer::timer1.EnablePwm<atmega_timer::Channel::kC>(512));
}
