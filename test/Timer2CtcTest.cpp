// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// timer2_ctc, the one object of Timer2 in CTC a sketch uses, and the compare
// match interrupt it is wired to. One test only: timer2_ctc lives as long as
// the program, which is one of its own, as timer2_ctc does not link beside
// timer2.

#include <avr/interrupt.h>
#include <gtest/gtest.h>

#include "atmega_timer/core.h"
#include "atmega_timer/status.h"
#include "atmega_timer/timer2_ctc.h"
#include "test/printers.h"  // IWYU pragma: keep

namespace atmega_timer {
namespace {

int match_calls{0};

void CountMatch() { ++match_calls; }

TEST(Timer2CtcTest, TheCompareMatchInterruptCallsTheCallbackOfTimer2Ctc) {
  ASSERT_EQ(timer2_ctc.GetState(), State::kNotBegun);
  ASSERT_EQ(timer2_ctc.Begin(1000U, CountMatch), Error::kOk);
  ASSERT_EQ(timer2_ctc.Start(), Error::kOk);

  TIMER2_COMPA_vect();

  EXPECT_EQ(match_calls, 1);
}

}  // namespace
}  // namespace atmega_timer
