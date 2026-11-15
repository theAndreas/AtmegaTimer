// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// timer1_ctc, Timer/Counter1 of the ATmega328P in CTC, at the Uno's 16 MHz:
// what its traits give Ctc, its 16-bit counter, its two channels and its
// interrupt. What CTC does on every timer alike CtcTest shows on Timer2. A
// program of its own, as timer1_ctc does not link beside timer1.

#include <avr/interrupt.h>
#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <string>

#include "Arduino.h"
#include "atmega_timer/core.h"
#include "atmega_timer/status.h"
#include "atmega_timer/timer1_ctc.h"
#include "test/printers.h"  // IWYU pragma: keep

namespace atmega_timer {
namespace {

using TimeType = Timer1Ctc::TimeType;

// The bits of TCCR1B that select the clock.
constexpr uint8_t kClockSelectMask{0x07};

int match_calls{0};

void CountMatch() { ++match_calls; }

bool BitIsSet(uint8_t reg, unsigned bit) {
  return ((static_cast<unsigned>(reg) >> bit) & 1u) != 0;
}

uint8_t ClockSelect() {
  return static_cast<uint8_t>(TCCR1B & kClockSelectMask);
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
  match_calls = 0;
}

class Timer1CtcTest : public ::testing::Test {
 protected:
  void SetUp() override { ResetTimer1(); }

  Timer1Ctc& timer() { return timer_; }

 private:
  Timer1Ctc timer_;
};

TEST_F(Timer1CtcTest, BeginSetsCtcWithICR1AsTheTop) {
  TCCR1A = 0xFF;
  TCCR1B = 0xFF;

  EXPECT_EQ(timer().Begin(), Error::kOk);

  // Mode 12 is WGM13 and WGM12; no output and no clock yet.
  EXPECT_EQ(TCCR1A, 0x00);
  EXPECT_EQ(TCCR1B, 0x18);
}

TEST_F(Timer1CtcTest, TheLongestPeriodIsAbout4Seconds) {
  // 65536 steps once at 16 MHz / 1024, half of phase correct PWM.
  EXPECT_EQ(Timer1Ctc::kPeriodMax, 4194304u);
}

TEST_F(Timer1CtcTest, APeriodBeyondTheLongestIsRefused) {
  EXPECT_EQ(timer().Begin(Timer1Ctc::kPeriodMax + 1u),
            Error::kPeriodOutOfRange);

  EXPECT_EQ(timer().GetState(), State::kNotBegun);
}

struct PeriodCase {
  char const* name;
  TimeType period;
  uint16_t top;
  uint8_t clock_select;
};

class Timer1CtcPeriodTest : public ::testing::TestWithParam<PeriodCase> {
 protected:
  void SetUp() override { ResetTimer1(); }

  Timer1Ctc& timer() { return timer_; }

 private:
  Timer1Ctc timer_;
};

// The period is (ICR1 + 1) * prescaler / 16 MHz: from 0 up to ICR1.
TEST_P(Timer1CtcPeriodTest, SetsTheTopAndTheSmallestPrescaler) {
  ASSERT_EQ(timer().Begin(GetParam().period), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(ICR1, GetParam().top);
  EXPECT_EQ(ClockSelect(), GetParam().clock_select);
}

INSTANTIATE_TEST_SUITE_P(
    EachPrescaler, Timer1CtcPeriodTest,
    ::testing::Values(PeriodCase{"By1", 1000u, 15999u, 1},
                      PeriodCase{"By8", 10000u, 19999u, 2},
                      PeriodCase{"By64", 100000u, 24999u, 3},
                      PeriodCase{"By256", 1000000u, 62499u, 4},
                      PeriodCase{"By1024", 4194304u, 65535u, 5}),
    [](::testing::TestParamInfo<PeriodCase> const& period_case) {
      return std::string{period_case.param.name};
    });

TEST_F(Timer1CtcTest, EnableToggleOfChannelAToggles9AtTheBottom) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  OCR1A = 20000;

  EXPECT_EQ(timer().EnableToggle<Channel::kA>(), Error::kOk);

  EXPECT_EQ(mock_pin_mode[9], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR1A, COM1A0));
  EXPECT_FALSE(BitIsSet(TCCR1A, COM1A1));
  EXPECT_EQ(OCR1A, 0);
}

TEST_F(Timer1CtcTest, EnableToggleOfChannelBToggles10AtTheBottom) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  OCR1B = 20000;

  EXPECT_EQ(timer().EnableToggle<Channel::kB>(), Error::kOk);

  EXPECT_EQ(mock_pin_mode[10], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR1A, COM1B0));
  EXPECT_FALSE(BitIsSet(TCCR1A, COM1B1));
  EXPECT_EQ(OCR1B, 0);
}

// In mode 12 the input capture flag marks the match at ICR1.
TEST_F(Timer1CtcTest, StartEnablesTheInputCaptureInterrupt) {
  ASSERT_EQ(timer().Begin(1000u, CountMatch), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK1, ICIE1));
  EXPECT_FALSE(BitIsSet(TIMSK1, TOIE1));
  EXPECT_TRUE(BitIsSet(TIFR1, ICF1));
}

// ICR1 is not buffered: while the counter runs, a new period waits for the
// match.
TEST_F(Timer1CtcTest, SetPeriodWhileRunningKeepsThePeriodUntilTheMatch) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);

  EXPECT_EQ(ICR1, 15999);
  EXPECT_EQ(ClockSelect(), 1);
  EXPECT_TRUE(BitIsSet(TIMSK1, ICIE1));
}

TEST_F(Timer1CtcTest, TheMatchTakesTheNewPeriod) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(10000u), Error::kOk);

  timer().OnInterrupt();

  EXPECT_EQ(ICR1, 19999);
  EXPECT_EQ(ClockSelect(), 2);
  EXPECT_FALSE(BitIsSet(TIMSK1, ICIE1));
}

// At 100 ms the prescaler is 64: a step of the counter is 4 us.
TEST_F(Timer1CtcTest, ReadCountsTheStepsSinceZero) {
  ASSERT_EQ(timer().Begin(100000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT1.Set(10000, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 40000u);
}

TEST_F(Timer1CtcTest, TheInputCaptureInterruptCallsTheCallbackOfTimer1Ctc) {
  ASSERT_EQ(timer1_ctc.Begin(1000u, CountMatch), Error::kOk);
  ASSERT_EQ(timer1_ctc.Start(), Error::kOk);

  TIMER1_CAPT_vect();

  EXPECT_EQ(match_calls, 1);
}

}  // namespace
}  // namespace atmega_timer
