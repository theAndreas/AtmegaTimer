// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// timer2, the one object a sketch uses, and the overflow interrupt it is
// wired to. One test only: timer2 lives as long as the program.

#include "atmega_timer/timer2.h"

#include <avr/interrupt.h>
#include <gtest/gtest.h>

#include "atmega_timer/core.h"
#include "atmega_timer/status.h"
#include "test/printers.h"  // IWYU pragma: keep

namespace atmega_timer {
namespace {

int overflow_calls{0};

void CountOverflow() { ++overflow_calls; }

TEST(Timer2Test, TheOverflowInterruptCallsTheCallbackOfTimer2) {
  ASSERT_EQ(timer2.GetState(), State::kNotBegun);
  ASSERT_EQ(timer2.Begin(1000U, CountOverflow), Error::kOk);
  ASSERT_EQ(timer2.Start(), Error::kOk);

  TIMER2_OVF_vect();

  EXPECT_EQ(overflow_calls, 1);
}

}  // namespace
}  // namespace atmega_timer
