// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The registers of Timer/Counter1 and Timer/Counter2 of the ATmega328P, for
// the tests on the host: each a variable the tests read and set, with the
// numbers of its bits as avr-libc names them. TCNT1 and TCNT2 count as the
// timers do in phase correct PWM, modes 8 and 5: one step on each read while a
// clock is selected, up from 0 to the top, ICR1 or OCR2A, and down again, so
// that what waits for the counter to move sees it move.

#ifndef ATMEGATIMER_TEST_MOCK_AVR_IO_H_
#define ATMEGATIMER_TEST_MOCK_AVR_IO_H_

#include <stdint.h>

// avr-libc's names of the bits, which are macros there; constants by the same
// names are all the library needs.
// NOLINTBEGIN(readability-identifier-naming)
constexpr int WGM10{0};
constexpr int WGM11{1};
constexpr int WGM12{3};
constexpr int WGM13{4};
constexpr int COM1B0{4};
constexpr int COM1B1{5};
constexpr int COM1A0{6};
constexpr int COM1A1{7};
constexpr int CS10{0};
constexpr int CS11{1};
constexpr int CS12{2};
constexpr int TOIE1{0};
constexpr int ICIE1{5};
constexpr int TOV1{0};
constexpr int ICF1{5};
constexpr int WGM20{0};
constexpr int WGM21{1};
constexpr int COM2B0{4};
constexpr int COM2B1{5};
constexpr int COM2A0{6};
constexpr int COM2A1{7};
constexpr int CS20{0};
constexpr int CS21{1};
constexpr int CS22{2};
constexpr int WGM22{3};
constexpr int TOIE2{0};
constexpr int OCIE2A{1};
constexpr int TOV2{0};
constexpr int OCF2A{1};
// NOLINTEND(readability-identifier-naming)

// TCNTn, which counts on each read while its TCCRnB selects a clock, up to
// its top and down again; Value is its width.
template <typename Value>
class MockCounter {
 public:
  // The counter of the timer whose clock select bits are in clock_select and
  // whose top is top.
  constexpr MockCounter(uint8_t volatile& clock_select, Value volatile& top)
      : clock_select_{&clock_select}, top_{&top} {}

  // The value the counter has now, after which it steps.
  operator Value();  // NOLINT(google-explicit-constructor)
  // Sets the counter, as a write to TCNTn does; it goes on upwards.
  MockCounter& operator=(Value value);

  // Sets the counter and its direction, for a test; it steps on each read.
  void Set(Value value, bool counting_up);

  // Lets the counter step only on every reads-th read, as a prescaler makes
  // the CPU read the same value several times.
  void SetReadsPerStep(uint8_t reads);

 private:
  uint8_t volatile* clock_select_;
  Value volatile* top_;
  Value value_{0};
  bool counting_up_{true};
  uint8_t reads_per_step_{1};
  uint8_t reads_{0};
};

// avr-libc's names of the registers, globals as they are on the board.
// NOLINTBEGIN(readability-identifier-naming)
extern uint8_t volatile TCCR1A;
extern uint8_t volatile TCCR1B;
extern uint16_t volatile ICR1;
extern uint16_t volatile OCR1A;
extern uint16_t volatile OCR1B;
extern uint8_t volatile TIMSK1;
// What was last written to TIFR1, as TIFR2.
extern uint8_t volatile TIFR1;
extern MockCounter<uint16_t> TCNT1;
extern uint8_t volatile TCCR2A;
extern uint8_t volatile TCCR2B;
extern uint8_t volatile OCR2A;
extern uint8_t volatile OCR2B;
extern uint8_t volatile TIMSK2;
// What was last written to TIFR2: on the board a 1 clears its flag, so a test
// sees which flags were cleared.
extern uint8_t volatile TIFR2;
extern MockCounter<uint8_t> TCNT2;
// NOLINTEND(readability-identifier-naming)

#endif  // ATMEGATIMER_TEST_MOCK_AVR_IO_H_
