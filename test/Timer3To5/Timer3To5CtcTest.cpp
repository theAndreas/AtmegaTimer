// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// timer3_ctc, timer4_ctc and timer5_ctc, Timer/Counter3 to 5 of the
// ATmega2560 in CTC, at the Mega's 16 MHz: what their registers give the core,
// the toggles of their three channels and their interrupts. One typed test for
// the three, as Timer3To5Test is; what every 16-bit timer does alike in CTC,
// Timer1CtcTest shows on Timer1.

#include <avr/interrupt.h>
#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <string>

#include "Arduino.h"
#include "atmega_timer/Modes/core.h"
#include "atmega_timer/Timer3/timer3_ctc.h"
#include "atmega_timer/Timer4/timer4_ctc.h"
#include "atmega_timer/Timer5/timer5_ctc.h"
#include "atmega_timer/status.h"
#include "test/Timer3To5/timers3_to_5.h"
#include "test/printers.h"  // IWYU pragma: keep

namespace atmega_timer {
namespace {

int match_calls{0};

void CountMatch() { ++match_calls; }

struct Timer3CtcCase : Timer3Case {
  using Timer = Timer3Ctc;
  static Timer& Object() { return timer3_ctc; }
  static void Interrupt() { TIMER3_CAPT_vect(); }
};

struct Timer4CtcCase : Timer4Case {
  using Timer = Timer4Ctc;
  static Timer& Object() { return timer4_ctc; }
  static void Interrupt() { TIMER4_CAPT_vect(); }
};

struct Timer5CtcCase : Timer5Case {
  using Timer = Timer5Ctc;
  static Timer& Object() { return timer5_ctc; }
  static void Interrupt() { TIMER5_CAPT_vect(); }
};

class CaseNames {
 public:
  template <typename Case>
  static std::string GetName(int index) {
    return "Timer" + std::to_string(index + 3);
  }
};

template <typename Case>
class Timer3To5CtcTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ResetRegisters<Case>();
    mock_pin_mode.fill(INPUT);
    match_calls = 0;
  }

  typename Case::Timer& timer() { return timer_; }

 private:
  typename Case::Timer timer_;
};

using Cases = ::testing::Types<Timer3CtcCase, Timer4CtcCase, Timer5CtcCase>;
TYPED_TEST_SUITE(Timer3To5CtcTest, Cases, CaseNames);

TYPED_TEST(Timer3To5CtcTest, BeginSetsCtcWithICRnAsTheTop) {
  TypeParam::Tccra() = 0xFF;
  TypeParam::Tccrb() = 0xFF;

  EXPECT_EQ(this->timer().Begin(), Error::kOk);

  // Mode 12 is WGMn3 and WGMn2; no output and no clock yet.
  EXPECT_EQ(TypeParam::Tccra(), 0x00);
  EXPECT_EQ(TypeParam::Tccrb(), 0x18);
}

// The counter counts from 0 to ICRn, one step more than the top.
TYPED_TEST(Timer3To5CtcTest, StartSetsTheTopAndThePrescaler) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);

  ASSERT_EQ(this->timer().Start(), Error::kOk);

  EXPECT_EQ(TypeParam::Icr(), 15999);
  // CSn0 alone: the CPU's clock.
  EXPECT_EQ(TypeParam::Tccrb(), 0x19);
}

TYPED_TEST(Timer3To5CtcTest, EnableToggleOfChannelATogglesItsPinAtTheBottom) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);
  TypeParam::OcrA() = 20000;

  EXPECT_EQ(this->timer().template EnableToggle<Channel::kA>(), Error::kOk);

  EXPECT_EQ(mock_pin_mode[TypeParam::kPinA], OUTPUT);
  // COMnA0, toggle on a match.
  EXPECT_EQ(TypeParam::Tccra(), 0x40);
  EXPECT_EQ(TypeParam::OcrA(), 0);
}

TYPED_TEST(Timer3To5CtcTest, EnableToggleOfChannelBTogglesItsPinAtTheBottom) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);
  TypeParam::OcrB() = 20000;

  EXPECT_EQ(this->timer().template EnableToggle<Channel::kB>(), Error::kOk);

  EXPECT_EQ(mock_pin_mode[TypeParam::kPinB], OUTPUT);
  // COMnB0, toggle on a match.
  EXPECT_EQ(TypeParam::Tccra(), 0x10);
  EXPECT_EQ(TypeParam::OcrB(), 0);
}

TYPED_TEST(Timer3To5CtcTest, EnableToggleOfChannelCTogglesItsPinAtTheBottom) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);
  TypeParam::OcrC() = 20000;

  EXPECT_EQ(this->timer().template EnableToggle<Channel::kC>(), Error::kOk);

  EXPECT_EQ(mock_pin_mode[TypeParam::kPinC], OUTPUT);
  // COMnC0, toggle on a match.
  EXPECT_EQ(TypeParam::Tccra(), 0x04);
  EXPECT_EQ(TypeParam::OcrC(), 0);
}

TYPED_TEST(Timer3To5CtcTest, DisableToggleOfChannelCDisconnectsIt) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(this->timer().template EnableToggle<Channel::kC>(), Error::kOk);

  this->timer().template DisableToggle<Channel::kC>();

  EXPECT_EQ(TypeParam::Tccra(), 0x00);
}

// In mode 12 the input capture flag marks the match at ICRn.
TYPED_TEST(Timer3To5CtcTest, StartEnablesTheInputCaptureInterrupt) {
  ASSERT_EQ(this->timer().Begin(1000u, CountMatch), Error::kOk);

  ASSERT_EQ(this->timer().Start(), Error::kOk);

  // ICIEn, and ICFn written to clear the flag.
  EXPECT_EQ(TypeParam::Timsk(), 0x20);
  EXPECT_EQ(TypeParam::Tifr(), 0x20);
}

// At 100 ms the prescaler is 64: a step of the counter is 4 us.
TYPED_TEST(Timer3To5CtcTest, ReadCountsTheStepsOfItsCounter) {
  ASSERT_EQ(this->timer().Begin(100000u), Error::kOk);
  ASSERT_EQ(this->timer().Start(), Error::kOk);
  TypeParam::Tcnt().Set(10000, true);

  Result<typename TypeParam::Timer::TimeType> const result{
      this->timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 40000u);
}

TYPED_TEST(Timer3To5CtcTest,
           TheInputCaptureInterruptCallsTheCallbackOfItsObject) {
  ASSERT_EQ(TypeParam::Object().Begin(1000u, CountMatch), Error::kOk);
  ASSERT_EQ(TypeParam::Object().Start(), Error::kOk);

  TypeParam::Interrupt();

  EXPECT_EQ(match_calls, 1);
}

}  // namespace
}  // namespace atmega_timer
