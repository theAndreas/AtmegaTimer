// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// timer3, timer4 and timer5, Timer/Counter3 to 5 of the ATmega2560 in phase
// and frequency correct PWM, at the Mega's 16 MHz: what their registers give
// the core, their three channels and their interrupts. One typed test for the
// three, as they differ in nothing but their registers and pins, rather than
// three files copied and changed a little (CodingStyle FN-13). What every
// 16-bit timer does alike, as a period that waits for the bottom, Timer1Test
// shows on Timer1.

#include <avr/interrupt.h>
#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <string>

#include "Arduino.h"
#include "atmega_timer/core.h"
#include "atmega_timer/status.h"
#include "atmega_timer/timer3.h"
#include "atmega_timer/timer4.h"
#include "atmega_timer/timer5.h"
#include "test/printers.h"  // IWYU pragma: keep
#include "test/timers3_to_5.h"

namespace atmega_timer {
namespace {

int overflow_calls{0};

void CountOverflow() { ++overflow_calls; }

struct Timer3Pwm : Timer3Case {
  using Timer = Timer3;
  static Timer& Object() { return timer3; }
  static void Interrupt() { TIMER3_OVF_vect(); }
};

struct Timer4Pwm : Timer4Case {
  using Timer = Timer4;
  static Timer& Object() { return timer4; }
  static void Interrupt() { TIMER4_OVF_vect(); }
};

struct Timer5Pwm : Timer5Case {
  using Timer = Timer5;
  static Timer& Object() { return timer5; }
  static void Interrupt() { TIMER5_OVF_vect(); }
};

class CaseNames {
 public:
  template <typename Case>
  static std::string GetName(int index) {
    return "Timer" + std::to_string(index + 3);
  }
};

template <typename Case>
class Timer3To5Test : public ::testing::Test {
 protected:
  void SetUp() override {
    ResetRegisters<Case>();
    mock_pin_mode.fill(INPUT);
    overflow_calls = 0;
  }

  typename Case::Timer& timer() { return timer_; }

 private:
  typename Case::Timer timer_;
};

using Cases = ::testing::Types<Timer3Pwm, Timer4Pwm, Timer5Pwm>;
TYPED_TEST_SUITE(Timer3To5Test, Cases, CaseNames);

TYPED_TEST(Timer3To5Test, BeginSetsPhaseAndFrequencyCorrectPwmWithoutAClock) {
  TypeParam::Tccra() = 0xFF;
  TypeParam::Tccrb() = 0xFF;

  EXPECT_EQ(this->timer().Begin(), Error::kOk);

  // Mode 8 is WGMn3 alone, with ICRn as the top; no output yet.
  EXPECT_EQ(TypeParam::Tccra(), 0x00);
  EXPECT_EQ(TypeParam::Tccrb(), 0x10);
}

TYPED_TEST(Timer3To5Test, TheLongestPeriodIsAbout8Seconds) {
  // 65536 steps up and down at 16 MHz / 1024.
  EXPECT_EQ(TypeParam::Timer::kPeriodMax, 8388607u);
}

// The period is 2 * ICRn * prescaler / 16 MHz: up to ICRn and down again.
TYPED_TEST(Timer3To5Test, StartSetsTheTopAndThePrescaler) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);

  ASSERT_EQ(this->timer().Start(), Error::kOk);

  EXPECT_EQ(TypeParam::Icr(), 8000);
  // CSn0 alone: the CPU's clock.
  EXPECT_EQ(TypeParam::Tccrb(), 0x11);
}

TYPED_TEST(Timer3To5Test, EnablePwmOfChannelADrivesItsPin) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(this->timer().template EnablePwm<Channel::kA>(512), Error::kOk);

  EXPECT_EQ(mock_pin_mode[TypeParam::kPinA], OUTPUT);
  // COMnA1, non-inverting.
  EXPECT_EQ(TypeParam::Tccra(), 0x80);
  // 512 / 1023 of the top, 8000, rounded.
  EXPECT_EQ(TypeParam::OcrA(), 4004);
}

TYPED_TEST(Timer3To5Test, EnablePwmOfChannelBDrivesItsPin) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(this->timer().template EnablePwm<Channel::kB>(256), Error::kOk);

  EXPECT_EQ(mock_pin_mode[TypeParam::kPinB], OUTPUT);
  // COMnB1, non-inverting.
  EXPECT_EQ(TypeParam::Tccra(), 0x20);
  // 256 / 1023 of the top, 8000, rounded.
  EXPECT_EQ(TypeParam::OcrB(), 2002);
}

TYPED_TEST(Timer3To5Test, EnablePwmOfChannelCDrivesItsPin) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(this->timer().template EnablePwm<Channel::kC>(768), Error::kOk);

  EXPECT_EQ(mock_pin_mode[TypeParam::kPinC], OUTPUT);
  // COMnC1, non-inverting.
  EXPECT_EQ(TypeParam::Tccra(), 0x08);
  // 768 / 1023 of the top, 8000, rounded.
  EXPECT_EQ(TypeParam::OcrC(), 6006);
}

TYPED_TEST(Timer3To5Test, DisablePwmOfChannelCDisconnectsIt) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(this->timer().template EnablePwm<Channel::kC>(768), Error::kOk);

  this->timer().template DisablePwm<Channel::kC>();

  EXPECT_EQ(TypeParam::Tccra(), 0x00);
}

TYPED_TEST(Timer3To5Test, SetPeriodKeepsTheShareOfTheDutyOfEachChannel) {
  ASSERT_EQ(this->timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(this->timer().template EnablePwm<Channel::kA>(512), Error::kOk);
  ASSERT_EQ(this->timer().template EnablePwm<Channel::kB>(256), Error::kOk);
  ASSERT_EQ(this->timer().template EnablePwm<Channel::kC>(768), Error::kOk);

  ASSERT_EQ(this->timer().SetPeriod(2000u), Error::kOk);

  // 512, 256 and 768 / 1023 of the new top, 16000, rounded.
  ASSERT_EQ(TypeParam::Icr(), 16000);
  EXPECT_EQ(TypeParam::OcrA(), 8008);
  EXPECT_EQ(TypeParam::OcrB(), 4004);
  EXPECT_EQ(TypeParam::OcrC(), 12012);
}

TYPED_TEST(Timer3To5Test, StartEnablesTheOverflowInterrupt) {
  ASSERT_EQ(this->timer().Begin(1000u, CountOverflow), Error::kOk);

  ASSERT_EQ(this->timer().Start(), Error::kOk);

  // TOIEn, and TOVn written to clear the flag.
  EXPECT_EQ(TypeParam::Timsk(), 0x01);
  EXPECT_EQ(TypeParam::Tifr(), 0x01);
}

// At 100 ms the prescaler is 64: a step of the counter is 4 us.
TYPED_TEST(Timer3To5Test, ReadCountsTheStepsOfItsCounter) {
  ASSERT_EQ(this->timer().Begin(100000u), Error::kOk);
  ASSERT_EQ(this->timer().Start(), Error::kOk);
  TypeParam::Tcnt().Set(10000, true);

  Result<typename TypeParam::Timer::TimeType> const result{
      this->timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 40000u);
}

TYPED_TEST(Timer3To5Test, TheOverflowInterruptCallsTheCallbackOfItsObject) {
  ASSERT_EQ(TypeParam::Object().Begin(1000u, CountOverflow), Error::kOk);
  ASSERT_EQ(TypeParam::Object().Start(), Error::kOk);

  TypeParam::Interrupt();

  EXPECT_EQ(overflow_calls, 1);
}

}  // namespace
}  // namespace atmega_timer
