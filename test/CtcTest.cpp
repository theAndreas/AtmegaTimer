// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Ctc on the registers of Timer2 in test/mock/avr/io.h, at the Uno's 16 MHz:
// each test makes a timer of its own, on registers set back to those after a
// reset. What every mode does alike, its states and its errors,
// PhaseCorrectPwmTest shows; here what CTC does otherwise.

#include "atmega_timer/ctc.h"

#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <string>

#include "Arduino.h"
#include "atmega_timer/core.h"
#include "atmega_timer/status.h"
#include "atmega_timer/timer2_traits.h"
#include "test/printers.h"  // IWYU pragma: keep

namespace atmega_timer {
namespace {

using Timer = Ctc<Timer2Traits>;
using TimeType = Timer::TimeType;

// The bits of TCCR2B that select the clock.
constexpr uint8_t kClockSelectMask{0x07};

int match_calls{0};

void CountMatch() { ++match_calls; }

bool BitIsSet(uint8_t reg, unsigned bit) {
  return ((static_cast<unsigned>(reg) >> bit) & 1U) != 0;
}

uint8_t ClockSelect() {
  return static_cast<uint8_t>(TCCR2B & kClockSelectMask);
}

void ResetTimer2() {
  TCCR2A = 0;
  TCCR2B = 0;
  OCR2A = 0;
  OCR2B = 0;
  TIMSK2 = 0;
  TIFR2 = 0;
  TCNT2.Set(0, true);
  mock_pin_mode.fill(INPUT);
  match_calls = 0;
}

class CtcTest : public ::testing::Test {
 protected:
  void SetUp() override { ResetTimer2(); }

  Timer& timer() { return timer_; }

 private:
  Timer timer_;
};

TEST_F(CtcTest, BeginSetsCtcWithoutAClock) {
  TCCR2A = 0xFF;
  TCCR2B = 0xFF;

  EXPECT_EQ(timer().Begin(), Error::kOk);

  // Mode 2 is WGM21 alone, with OCR2A as the top; no output yet.
  EXPECT_EQ(TCCR2A, 0x02);
  EXPECT_EQ(TCCR2B, 0x00);
}

TEST_F(CtcTest, TheLongestPeriodIsAbout16Milliseconds) {
  // 256 steps once at 16 MHz / 1024, half of phase correct PWM.
  EXPECT_EQ(Timer::kPeriodMax, 16384U);
}

TEST_F(CtcTest, TheLongestPeriodCountsEveryValueOfTheCounter) {
  ASSERT_EQ(timer().Begin(Timer::kPeriodMax), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(OCR2A, 255);
  EXPECT_EQ(ClockSelect(), 7);
}

TEST_F(CtcTest, APeriodBeyondTheLongestIsRefused) {
  EXPECT_EQ(timer().Begin(static_cast<TimeType>(Timer::kPeriodMax + 1U)),
            Error::kPeriodOutOfRange);

  EXPECT_EQ(timer().GetState(), State::kNotBegun);
}

// Far beyond, no prescaler lets the period fit the counter.
TEST_F(CtcTest, APeriodBeyondEveryPrescalerIsRefused) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  EXPECT_EQ(timer().SetPeriod(65535U), Error::kPeriodOutOfRange);

  EXPECT_EQ(OCR2A, 249);
}

TEST_F(CtcTest, APeriodOfZeroIsRefused) {
  EXPECT_EQ(timer().Begin(0U), Error::kPeriodOutOfRange);

  EXPECT_EQ(timer().GetState(), State::kNotBegun);
}

struct PeriodCase {
  char const* name;
  TimeType period;
  uint8_t top;
  uint8_t clock_select;
};

class CtcPeriodTest : public ::testing::TestWithParam<PeriodCase> {
 protected:
  void SetUp() override { ResetTimer2(); }

  Timer& timer() { return timer_; }

 private:
  Timer timer_;
};

// The period is (OCR2A + 1) * prescaler / 16 MHz: from 0 up to OCR2A.
TEST_P(CtcPeriodTest, SetsTheTopAndTheSmallestPrescaler) {
  ASSERT_EQ(timer().Begin(GetParam().period), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(OCR2A, GetParam().top);
  EXPECT_EQ(ClockSelect(), GetParam().clock_select);
}

INSTANTIATE_TEST_SUITE_P(
    EachPrescaler, CtcPeriodTest,
    ::testing::Values(PeriodCase{"By1", 10U, 159, 1},
                      PeriodCase{"By1AtItsLongest", 16U, 255, 1},
                      PeriodCase{"By8", 17U, 33, 2},
                      PeriodCase{"By32", 500U, 249, 3},
                      PeriodCase{"By64", 1000U, 249, 4},
                      PeriodCase{"By128", 2000U, 249, 5},
                      PeriodCase{"By256", 4000U, 249, 6},
                      PeriodCase{"By1024", 10000U, 155, 7}),
    [](::testing::TestParamInfo<PeriodCase> const& period_case) {
      return std::string{period_case.param.name};
    });

TEST_F(CtcTest, EnableToggleOfChannelBToggles3AtTheBottom) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  OCR2B = 200;

  EXPECT_EQ(timer().EnableToggle<Channel::kB>(), Error::kOk);

  EXPECT_EQ(mock_pin_mode[3], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2B0));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
  // At 0 the counter is in every period, whatever its top.
  EXPECT_EQ(OCR2B, 0);
}

TEST_F(CtcTest, EnableToggleOfChannelAToggles11AtTheTop) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  EXPECT_EQ(timer().EnableToggle<Channel::kA>(), Error::kOk);

  EXPECT_EQ(mock_pin_mode[11], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2A0));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2A1));
  // OCR2A is the top, which the toggle leaves as it is.
  EXPECT_EQ(OCR2A, 249);
}

TEST_F(CtcTest, EnableToggleIsRefusedBeforeBegin) {
  OCR2B = 7;

  EXPECT_EQ(timer().EnableToggle<Channel::kB>(), Error::kNotBegun);

  EXPECT_EQ(TCCR2A, 0);
  EXPECT_EQ(mock_pin_mode[3], INPUT);
  EXPECT_EQ(OCR2B, 7);
}

TEST_F(CtcTest, SetPeriodLeavesTheToggleAtTheBottom) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().EnableToggle<Channel::kB>(), Error::kOk);

  ASSERT_EQ(timer().SetPeriod(100U), Error::kOk);

  EXPECT_EQ(OCR2A, 199);
  EXPECT_EQ(OCR2B, 0);
}

TEST_F(CtcTest, DisableToggleLeavesTheOtherChannel) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().EnableToggle<Channel::kA>(), Error::kOk);
  ASSERT_EQ(timer().EnableToggle<Channel::kB>(), Error::kOk);

  timer().DisableToggle<Channel::kB>();

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B0));
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2A0));
}

TEST_F(CtcTest, StartEnablesTheInterruptOfTheMatch) {
  ASSERT_EQ(timer().Begin(1000U, CountMatch), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK2, OCIE2A));
  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
  // Written with a 1, which clears it.
  EXPECT_TRUE(BitIsSet(TIFR2, OCF2A));
}

TEST_F(CtcTest, TheMatchCallsTheCallback) {
  ASSERT_EQ(timer().Begin(1000U, CountMatch), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  timer().OnInterrupt();

  EXPECT_EQ(match_calls, 1);
}

// OCR2A is not buffered in CTC: a top written below the counter would let it
// run on to 255. While it counts, a new period waits for the match.
TEST_F(CtcTest, SetPeriodWhileRunningKeepsThePeriodUntilTheMatch) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().SetPeriod(100U), Error::kOk);

  EXPECT_EQ(OCR2A, 249);
  EXPECT_EQ(ClockSelect(), 4);
  EXPECT_TRUE(BitIsSet(TIMSK2, OCIE2A));
}

TEST_F(CtcTest, TheMatchTakesTheNewPeriod) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().SetPeriod(100U), Error::kOk);

  timer().OnInterrupt();

  EXPECT_EQ(OCR2A, 199);
  EXPECT_EQ(ClockSelect(), 2);
  // No callback: the interrupt was for the period alone.
  EXPECT_FALSE(BitIsSet(TIMSK2, OCIE2A));
}

TEST_F(CtcTest, ReadIsRefusedUnlessRunning) {
  ASSERT_EQ(timer().Begin(100U), Error::kOk);

  Result<TimeType> const result{timer().Read()};

  EXPECT_EQ(result.GetError(), Error::kNotRunning);
}

// At 100 us the prescaler is 8: a step of the counter is 0.5 us.
TEST_F(CtcTest, ReadCountsTheStepsSinceZero) {
  ASSERT_EQ(timer().Begin(100U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 25U);
}

// The counter only counts up: one read, without waiting for a step to see
// which way it goes.
TEST_F(CtcTest, ReadTakesTheCounterAsItStands) {
  ASSERT_EQ(timer().Begin(100U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, false);
  TCNT2.SetReadsPerStep(4);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 25U);
}

}  // namespace
}  // namespace atmega_timer
