// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The registers and pins of Timer3, Timer4 and Timer5 of the ATmega2560, for
// the typed tests of Timer3To5Test and Timer3To5CtcTest: the three timers
// differ in nothing but these, so one test of each behaviour runs on all
// three. The pins are written down as the core's pins_arduino.h of the Mega
// has them, not taken from the traits, so that a test shows a pin the traits
// get wrong.

#ifndef ATMEGATIMER_TEST_TIMER3TO5_TIMERS3_TO_5_H_
#define ATMEGATIMER_TEST_TIMER3TO5_TIMERS3_TO_5_H_

#include <avr/io.h>
#include <stdint.h>

namespace atmega_timer {

struct Timer3Case {
  static constexpr uint8_t kPinA{5};  // PE3
  static constexpr uint8_t kPinB{2};  // PE4
  static constexpr uint8_t kPinC{3};  // PE5

  static uint8_t volatile& Tccra() { return TCCR3A; }
  static uint8_t volatile& Tccrb() { return TCCR3B; }
  static uint16_t volatile& Icr() { return ICR3; }
  static uint16_t volatile& OcrA() { return OCR3A; }
  static uint16_t volatile& OcrB() { return OCR3B; }
  static uint16_t volatile& OcrC() { return OCR3C; }
  static uint8_t volatile& Timsk() { return TIMSK3; }
  static uint8_t volatile& Tifr() { return TIFR3; }
  static MockCounter<uint16_t>& Tcnt() { return TCNT3; }
};

struct Timer4Case {
  static constexpr uint8_t kPinA{6};  // PH3
  static constexpr uint8_t kPinB{7};  // PH4
  static constexpr uint8_t kPinC{8};  // PH5

  static uint8_t volatile& Tccra() { return TCCR4A; }
  static uint8_t volatile& Tccrb() { return TCCR4B; }
  static uint16_t volatile& Icr() { return ICR4; }
  static uint16_t volatile& OcrA() { return OCR4A; }
  static uint16_t volatile& OcrB() { return OCR4B; }
  static uint16_t volatile& OcrC() { return OCR4C; }
  static uint8_t volatile& Timsk() { return TIMSK4; }
  static uint8_t volatile& Tifr() { return TIFR4; }
  static MockCounter<uint16_t>& Tcnt() { return TCNT4; }
};

struct Timer5Case {
  static constexpr uint8_t kPinA{46};  // PL3
  static constexpr uint8_t kPinB{45};  // PL4
  static constexpr uint8_t kPinC{44};  // PL5

  static uint8_t volatile& Tccra() { return TCCR5A; }
  static uint8_t volatile& Tccrb() { return TCCR5B; }
  static uint16_t volatile& Icr() { return ICR5; }
  static uint16_t volatile& OcrA() { return OCR5A; }
  static uint16_t volatile& OcrB() { return OCR5B; }
  static uint16_t volatile& OcrC() { return OCR5C; }
  static uint8_t volatile& Timsk() { return TIMSK5; }
  static uint8_t volatile& Tifr() { return TIFR5; }
  static MockCounter<uint16_t>& Tcnt() { return TCNT5; }
};

// The registers of a case as after a reset.
template <typename Case>
void ResetRegisters() {
  Case::Tccra() = 0;
  Case::Tccrb() = 0;
  Case::Icr() = 0;
  Case::OcrA() = 0;
  Case::OcrB() = 0;
  Case::OcrC() = 0;
  Case::Timsk() = 0;
  Case::Tifr() = 0;
  Case::Tcnt().Set(0, true);
}

}  // namespace atmega_timer

#endif  // ATMEGATIMER_TEST_TIMER3TO5_TIMERS3_TO_5_H_
