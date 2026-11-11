// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "avr/io.h"

#include <stdint.h>

uint8_t volatile TCCR1A{0};
uint8_t volatile TCCR1B{0};
uint16_t volatile ICR1{0};
uint16_t volatile OCR1A{0};
uint16_t volatile OCR1B{0};
uint8_t volatile TIMSK1{0};
uint8_t volatile TIFR1{0};
MockCounter<uint16_t> TCNT1{TCCR1B, ICR1};
uint8_t volatile TCCR2A{0};
uint8_t volatile TCCR2B{0};
uint8_t volatile OCR2A{0};
uint8_t volatile OCR2B{0};
uint8_t volatile TIMSK2{0};
uint8_t volatile TIFR2{0};
MockCounter<uint8_t> TCNT2{TCCR2B, OCR2A};

namespace {

// The clock select bits CSn2..CSn0 of TCCRnB.
constexpr uint8_t kClockSelectMask{0x07};

}  // namespace

template <typename Value>
MockCounter<Value>::operator Value() {
  Value const now{value_};
  ++reads_;
  if (((*clock_select_ & kClockSelectMask) != 0) &&
      (reads_ >= reads_per_step_)) {
    reads_ = 0;
    Value const top{*top_};
    if (counting_up_ && value_ >= top) {
      counting_up_ = false;
    } else if (!counting_up_ && value_ == 0) {
      counting_up_ = true;
    }
    if (top != 0) {
      value_ = counting_up_ ? static_cast<Value>(value_ + 1)
                            : static_cast<Value>(value_ - 1);
    }
  }
  return now;
}

template <typename Value>
MockCounter<Value>& MockCounter<Value>::operator=(Value value) {
  Set(value, true);
  return *this;
}

template <typename Value>
void MockCounter<Value>::Set(Value value, bool counting_up) {
  value_ = value;
  counting_up_ = counting_up;
  reads_per_step_ = 1;
  reads_ = 0;
}

template <typename Value>
void MockCounter<Value>::SetReadsPerStep(uint8_t reads) {
  reads_per_step_ = reads;
  reads_ = 0;
}

// The two widths of the AVR's counters.
template class MockCounter<uint8_t>;
template class MockCounter<uint16_t>;
