// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// timer1, Timer/Counter1 of the ATmega328P in phase and frequency correct PWM,
// at the Uno's 16 MHz: what its traits give the core, its 16-bit counter, its
// two channels and its interrupt. What every timer does alike, its states and
// its errors, PhaseCorrectPwmTest shows on Timer2.

#include "atmega_timer/timer1.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <string>

#include "Arduino.h"
#include "atmega_timer/Bit.h"
#include "atmega_timer/core.h"
#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/status.h"
#include "test/pins.h"
#include "test/printers.h"  // IWYU pragma: keep

namespace atmega_timer {
namespace {

using TimeType = Timer1::TimeType;

// The bits of TCCR1B that select the clock.
constexpr uint8_t kClockSelectMask{0x07};

int overflow_calls{0};

void CountOverflow() { ++overflow_calls; }

bool BitIsSet(uint8_t reg, unsigned bit) {
  return Bit<uint8_t>::ReadBit(reg, static_cast<uint8_t>(bit));
}

uint8_t ClockSelect() {
  return Bit<uint8_t>::ReadBitGroup(TCCR1B, kClockSelectMask);
}

void ResetTimer1() {
  TCCR1A = 0;
  TCCR1B = 0;
  ICR1 = 0;
  OCR1A = 0;
  OCR1B = 0;
  TIMSK1 = 0;
  TIFR1 = 0;
  TCNT1.Set(0, true);
  mock_pin_mode.fill(INPUT);
  overflow_calls = 0;
}

class Timer1Test : public ::testing::Test {
 protected:
  void SetUp() override { ResetTimer1(); }

  Timer1& timer() { return timer_; }

 private:
  Timer1 timer_;
};

TEST_F(Timer1Test, BeginSetsPhaseAndFrequencyCorrectPwmWithoutAClock) {
  TCCR1A = 0xFF;
  TCCR1B = 0xFF;

  EXPECT_EQ(timer().Begin(), Error::kOk);

  // Mode 8 is WGM13 alone, with ICR1 as the top; no output yet.
  EXPECT_EQ(TCCR1A, 0x00);
  EXPECT_EQ(TCCR1B, 0x10);
}

TEST_F(Timer1Test, TheLongestPeriodIsAbout8Seconds) {
  // 65536 steps up and down at 16 MHz / 1024.
  EXPECT_EQ(Timer1::kPeriodMax, 8388607u);
}

TEST_F(Timer1Test, APeriodBeyondTheLongestIsRefused) {
  EXPECT_EQ(timer().Begin(Timer1::kPeriodMax + 1u), Error::kPeriodOutOfRange);

  EXPECT_EQ(timer().GetState(), State::kNotBegun);
}

struct PeriodCase {
  char const* name;
  TimeType period;
  uint16_t top;
  uint8_t clock_select;
};

class Timer1PeriodTest : public ::testing::TestWithParam<PeriodCase> {
 protected:
  void SetUp() override { ResetTimer1(); }

  Timer1& timer() { return timer_; }

 private:
  Timer1 timer_;
};

// The period is 2 * ICR1 * prescaler / 16 MHz: up to ICR1 and down again.
TEST_P(Timer1PeriodTest, SetsTheTopAndTheSmallestPrescaler) {
  ASSERT_EQ(timer().Begin(GetParam().period), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(ICR1, GetParam().top);
  EXPECT_EQ(ClockSelect(), GetParam().clock_select);
}

INSTANTIATE_TEST_SUITE_P(
    EachPrescaler, Timer1PeriodTest,
    ::testing::Values(PeriodCase{"By1", 1000u, 8000u, 1},
                      PeriodCase{"By8", 10000u, 10000u, 2},
                      PeriodCase{"By64", 100000u, 12500u, 3},
                      PeriodCase{"By256", 1000000u, 31250u, 4},
                      PeriodCase{"By1024", 8388607u, 65535u, 5}),
    [](::testing::TestParamInfo<PeriodCase> const& period_case) {
      return std::string{period_case.param.name};
    });

TEST_F(Timer1Test, EnablePwmOfChannelADrivesOc1a) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(timer().EnablePwm<Channel::kA>(512), Error::kOk);

  EXPECT_EQ(mock_pin_mode[kOc1aPin], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR1A, COM1A1));
  EXPECT_FALSE(BitIsSet(TCCR1A, COM1A0));
  // 512 / 1023 of the top, 8000, rounded.
  EXPECT_EQ(OCR1A, 4004);
}

TEST_F(Timer1Test, EnablePwmOfChannelBDrivesOc1b) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(timer().EnablePwm<Channel::kB>(256), Error::kOk);

  EXPECT_EQ(mock_pin_mode[kOc1bPin], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR1A, COM1B1));
  EXPECT_FALSE(BitIsSet(TCCR1A, COM1B0));
  // 256 / 1023 of the top, 8000, rounded.
  EXPECT_EQ(OCR1B, 2002);
}

TEST_F(Timer1Test, AFullDutyIsTheTop) {
  ASSERT_EQ(timer().Begin(Timer1::kPeriodMax), Error::kOk);

  ASSERT_EQ(timer().EnablePwm<Channel::kA>(kDutyMax), Error::kOk);

  EXPECT_EQ(OCR1A, 65535);
}

TEST_F(Timer1Test, SetPeriodKeepsTheShareOfTheDutyOfEachChannel) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().EnablePwm<Channel::kA>(512), Error::kOk);
  ASSERT_EQ(timer().EnablePwm<Channel::kB>(256), Error::kOk);

  ASSERT_EQ(timer().SetPeriod(2000u), Error::kOk);

  // 512 and 256 / 1023 of the new top, 16000, rounded.
  ASSERT_EQ(ICR1, 16000);
  EXPECT_EQ(OCR1A, 8008);
  EXPECT_EQ(OCR1B, 4004);
}

TEST_F(Timer1Test, StartEnablesTheOverflowInterrupt) {
  ASSERT_EQ(timer().Begin(1000u, CountOverflow), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK1, TOIE1));
  EXPECT_TRUE(BitIsSet(TIFR1, TOV1));
}

// At 100 ms the prescaler is 64: a step of the counter is 4 us.
TEST_F(Timer1Test, ReadCountsTheStepsUpwards) {
  ASSERT_EQ(timer().Begin(100000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT1.Set(10000, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 40000u);
}

// At the longest period the steps up and back down exceed 16 bits, and
// their clocks times 1000 exceed 32.
TEST_F(Timer1Test, ReadHoldsTheStepsOfTheLongestPeriod) {
  ASSERT_EQ(timer().Begin(Timer1::kPeriodMax), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  // 65535 steps up and 65435 down, of 64 us.
  TCNT1.Set(100, false);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 8382080u);
}

// ICR1 is not buffered: a top written below the counter would let it run on
// to 65535. While it counts, a new period waits for the bottom.
TEST_F(Timer1Test, SetPeriodWhileRunningKeepsThePeriodUntilTheBottom) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().SetPeriod(10000u), Error::kOk);

  EXPECT_EQ(ICR1, 8000);
  EXPECT_EQ(ClockSelect(), 1);
  EXPECT_TRUE(BitIsSet(TIMSK1, TOIE1));
  EXPECT_TRUE(BitIsSet(TIFR1, TOV1));
}

TEST_F(Timer1Test, TheBottomTakesTheNewPeriod) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);

  timer().OnInterrupt();

  EXPECT_EQ(ICR1, 10000);
  EXPECT_EQ(ClockSelect(), 2);
  // No callback: the interrupt was for the period alone.
  EXPECT_FALSE(BitIsSet(TIMSK1, TOIE1));
}

TEST_F(Timer1Test, TheBottomKeepsTheInterruptOfACallback) {
  ASSERT_EQ(timer().Begin(1000u, CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);

  timer().OnInterrupt();

  EXPECT_EQ(ICR1, 10000);
  EXPECT_TRUE(BitIsSet(TIMSK1, TOIE1));
  EXPECT_EQ(overflow_calls, 1);
}

// The compare values are buffered: the hardware takes them at the bottom,
// with the new top.
TEST_F(Timer1Test, SetPeriodWhileRunningWritesTheCompareValuesAtOnce) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().EnablePwm<Channel::kA>(512), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  ASSERT_EQ(timer().SetPeriod(2000u), Error::kOk);

  // 512 / 1023 of the new top, 16000, rounded.
  EXPECT_EQ(OCR1A, 8008);
  EXPECT_EQ(ICR1, 8000);
}

TEST_F(Timer1Test, ADutyBeforeTheBottomIsAShareOfTheNewTop) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(2000u), Error::kOk);

  ASSERT_EQ(timer().EnablePwm<Channel::kA>(512), Error::kOk);

  // 512 / 1023 of the new top, 16000, rounded.
  EXPECT_EQ(OCR1A, 8008);
}

// The counter runs in the old period until the bottom: a step is still a
// clock, 1/16 us, not 64 of them.
TEST_F(Timer1Test, ReadBeforeTheBottomCountsInTheOldPeriod) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(100000u), Error::kOk);
  TCNT1.Set(800, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 50u);
}

TEST_F(Timer1Test, DetachInterruptBeforeTheBottomKeepsItForThePeriod) {
  ASSERT_EQ(timer().Begin(1000u, CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);

  timer().DetachInterrupt();

  EXPECT_TRUE(BitIsSet(TIMSK1, TOIE1));
  timer().OnInterrupt();
  EXPECT_EQ(ICR1, 10000);
  EXPECT_FALSE(BitIsSet(TIMSK1, TOIE1));
  EXPECT_EQ(overflow_calls, 0);
}

// Stopped, the counter may stand above the new top: Resume() would let it run
// on. The period waits for the bottom there too.
TEST_F(Timer1Test, SetPeriodWhileStoppedKeepsThePeriodUntilTheBottom) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);

  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);
  ASSERT_EQ(timer().Resume(), Error::kOk);

  EXPECT_EQ(ICR1, 8000);
  EXPECT_EQ(ClockSelect(), 1);
  EXPECT_TRUE(BitIsSet(TIMSK1, TOIE1));
}

TEST_F(Timer1Test, AttachInterruptWhileStoppedKeepsItForThePeriod) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);

  ASSERT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK1, TOIE1));
}

// Start() begins at the bottom, so it takes a period that waits at once.
TEST_F(Timer1Test, StartTakesAWaitingPeriodAtOnce) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(ICR1, 10000);
  EXPECT_EQ(ClockSelect(), 2);
  EXPECT_FALSE(BitIsSet(TIMSK1, TOIE1));
}

TEST_F(Timer1Test, SetPeriodBeforeStartTakesThePeriodAtOnce) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);

  EXPECT_EQ(ICR1, 10000);
  EXPECT_FALSE(BitIsSet(TIMSK1, TOIE1));
}

TEST_F(Timer1Test, TheOverflowInterruptCallsTheCallbackOfTimer1) {
  ASSERT_EQ(timer1.Begin(1000u, CountOverflow), Error::kOk);
  ASSERT_EQ(timer1.Start(), Error::kOk);

  TIMER1_OVF_vect();

  EXPECT_EQ(overflow_calls, 1);
}

}  // namespace
}  // namespace atmega_timer
