// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Timer at whatever clock F_CPU is: CMakeLists.txt builds these tests at 1,
// 16 and 20 MHz. Each expectation is computed from F_CPU in 64 bits, apart
// from how Timer computes it; on Timer2, with a counter of 8 bits, and on
// Timer1, with one of 16.

#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <array>
#include <string>

#include "atmega_timer/ctc.h"
#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/status.h"
#include "atmega_timer/timer1.h"
#include "atmega_timer/timer1_traits.h"
#include "atmega_timer/timer2.h"
#include "atmega_timer/timer2_traits.h"
#include "test/printers.h"  // IWYU pragma: keep

namespace atmega_timer {
namespace {

using TimeType = Timer2::TimeType;

constexpr uint64_t kClock{F_CPU};
constexpr uint64_t kMicrosecondsPerSecond{1000000};

// Mode 5 counts up to the top and down again, 2 * 256 steps at the most, of
// the largest prescaler.
constexpr uint64_t kLongestPeriodInClocks{uint64_t{2} * 256 * 1024};

// The prescaler TCCR2B selects, 0 while the timer does not run.
uint64_t Prescaler() {
  constexpr std::array<uint64_t, 8> kPrescalers{0,  1,   8,   32,
                                                64, 128, 256, 1024};
  return kPrescalers.at(TCCR2B & 0x07U);
}

// The period OCR2A and TCCR2B make, in clocks.
uint64_t PeriodInClocks() { return uint64_t{2} * OCR2A * Prescaler(); }

class ClockTest : public ::testing::Test {
 protected:
  void SetUp() override {
    TCCR2A = 0;
    TCCR2B = 0;
    OCR2A = 0;
    TCNT2.Set(0, true);
  }

  Timer2& timer() { return timer_; }

 private:
  Timer2 timer_;
};

TEST_F(ClockTest, TheLongestPeriodIsTheLastThatFits) {
  uint64_t const longest{
      ((kLongestPeriodInClocks * kMicrosecondsPerSecond) - 1) / kClock};

  EXPECT_EQ(Timer2::kPeriodMax, longest);
}

TEST_F(ClockTest, TheLongestPeriodIsTakenAndOneMoreRefused) {
  ASSERT_EQ(timer().Begin(Timer2::kPeriodMax), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(Prescaler(), 1024U);

  EXPECT_EQ(timer().SetPeriod(static_cast<TimeType>(Timer2::kPeriodMax + 1U)),
            Error::kPeriodOutOfRange);
}

// Each period is met to less than a step of the counter up and one down, and
// the two clocks that dividing by 2 and by the prescaler may cut.
TEST_F(ClockTest, EachPeriodIsMetWithinAStep) {
  ASSERT_EQ(timer().Begin(), Error::kOk);
  // The prescaler is in TCCR2B while the timer runs.
  ASSERT_EQ(timer().Start(), Error::kOk);
  constexpr std::array<uint32_t, 8> kPeriods{2,    3,     10,    100,
                                             1000, 10000, 20000, 26214};

  for (uint32_t const period : kPeriods) {
    ASSERT_EQ(timer().SetPeriod(static_cast<TimeType>(period)), Error::kOk)
        << period << " us";
    uint64_t const wanted{period * kClock / kMicrosecondsPerSecond};

    EXPECT_LE(PeriodInClocks(), wanted) << period << " us";
    EXPECT_LT(wanted - PeriodInClocks(), (2 * Prescaler()) + 2)
        << period << " us";
  }
}

TEST_F(ClockTest, APeriodShorterThanAStepIsRefused) {
  // One step up and one down at no prescaler: two clocks.
  constexpr bool kOneMicrosecondIsAStep{2 * kMicrosecondsPerSecond <= kClock};

  EXPECT_EQ(timer().Begin(1U),
            kOneMicrosecondIsAStep ? Error::kOk : Error::kPeriodOutOfRange);
}

TEST_F(ClockTest, ReadIsTheTimeOfTheStepsCounted) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(),
            50 * Prescaler() * kMicrosecondsPerSecond / kClock);
}

// Mode 8 counts up to the top and down again, 2 * 65536 steps at the most, of
// the largest prescaler.
constexpr uint64_t kLongestPeriodOfTimer1InClocks{uint64_t{2} * 65536 * 1024};

// The prescaler TCCR1B selects, 0 while the timer does not run.
uint64_t PrescalerOfTimer1() {
  constexpr std::array<uint64_t, 6> kPrescalers{0, 1, 8, 64, 256, 1024};
  return kPrescalers.at(TCCR1B & 0x07U);
}

// The period ICR1 and TCCR1B make, in clocks.
uint64_t PeriodOfTimer1InClocks() {
  return uint64_t{2} * ICR1 * PrescalerOfTimer1();
}

class Timer1ClockTest : public ::testing::Test {
 protected:
  void SetUp() override {
    TCCR1A = 0;
    TCCR1B = 0;
    ICR1 = 0;
    TCNT1.Set(0, true);
  }

  Timer1& timer() { return timer_; }

 private:
  Timer1 timer_;
};

TEST_F(Timer1ClockTest, TheLongestPeriodIsTheLastThatFits) {
  uint64_t const longest{
      ((kLongestPeriodOfTimer1InClocks * kMicrosecondsPerSecond) - 1) / kClock};

  EXPECT_EQ(Timer1::kPeriodMax, longest);
}

TEST_F(Timer1ClockTest, TheLongestPeriodIsTakenAndOneMoreRefused) {
  ASSERT_EQ(timer().Begin(Timer1::kPeriodMax), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(PrescalerOfTimer1(), 1024U);

  EXPECT_EQ(timer().SetPeriod(Timer1::kPeriodMax + 1U),
            Error::kPeriodOutOfRange);
}

// The steps up from the top and back down beyond 16 bits, of the prescaler of
// a period of 4 s.
TEST_F(Timer1ClockTest, ReadIsTheTimeOfTheStepsCounted) {
  ASSERT_EQ(timer().Begin(4000000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT1.Set(10, false);

  Result<Timer1::TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  uint64_t const steps{(uint64_t{2} * ICR1) - 10};
  EXPECT_EQ(result.GetValue(),
            steps * PrescalerOfTimer1() * kMicrosecondsPerSecond / kClock);
}

class Timer1PeriodAtClockTest : public ::testing::TestWithParam<uint32_t> {
 protected:
  void SetUp() override {
    TCCR1A = 0;
    TCCR1B = 0;
    ICR1 = 0;
    TCNT1.Set(0, true);
  }

  Timer1& timer() { return timer_; }

 private:
  Timer1 timer_;
};

// Each period is met to less than a step of the counter up and one down, and
// the two clocks that dividing by 2 and by the prescaler may cut.
TEST_P(Timer1PeriodAtClockTest, IsMetWithinAStep) {
  ASSERT_EQ(timer().Begin(GetParam()), Error::kOk);

  // The prescaler is in TCCR1B while the timer runs.
  ASSERT_EQ(timer().Start(), Error::kOk);

  uint64_t const wanted{GetParam() * kClock / kMicrosecondsPerSecond};
  EXPECT_LE(PeriodOfTimer1InClocks(), wanted);
  EXPECT_LT(wanted - PeriodOfTimer1InClocks(), (2 * PrescalerOfTimer1()) + 2);
}

// From two clocks at 1 MHz to the longest period at 20 MHz, about 6.7 s.
INSTANTIATE_TEST_SUITE_P(FromTwoClocksToSeconds, Timer1PeriodAtClockTest,
                         ::testing::Values(2U, 3U, 10U, 1000U, 100000U,
                                           1000000U, 4194303U, 6710886U),
                         [](::testing::TestParamInfo<uint32_t> const& period) {
                           return std::to_string(period.param) + "us";
                         });

// CTC counts the period once, 256 steps at the most of the largest prescaler.
constexpr uint64_t kLongestCtcPeriodInClocks{uint64_t{256} * 1024};

// The period OCR2A and TCCR2B make in CTC, in clocks: from 0 up to OCR2A.
uint64_t CtcPeriodInClocks() { return (uint64_t{OCR2A} + 1) * Prescaler(); }

class CtcClockTest : public ::testing::Test {
 protected:
  void SetUp() override {
    TCCR2A = 0;
    TCCR2B = 0;
    OCR2A = 0;
    TCNT2.Set(0, true);
  }

  Ctc<Timer2Traits>& timer() { return timer_; }

 private:
  Ctc<Timer2Traits> timer_;
};

TEST_F(CtcClockTest, TheLongestPeriodIsTheLastThatFits) {
  uint64_t const longest{(kLongestCtcPeriodInClocks * kMicrosecondsPerSecond) /
                         kClock};

  EXPECT_EQ(Ctc<Timer2Traits>::kPeriodMax, longest);
}

TEST_F(CtcClockTest, TheLongestPeriodIsTakenAndOneMoreRefused) {
  ASSERT_EQ(timer().Begin(Ctc<Timer2Traits>::kPeriodMax), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(Prescaler(), 1024U);

  EXPECT_EQ(timer().SetPeriod(
                static_cast<TimeType>(Ctc<Timer2Traits>::kPeriodMax + 1U)),
            Error::kPeriodOutOfRange);
}

// Each period is met to less than a step of the counter: the clocks of the
// microseconds are cut first, then the clocks of the prescaler.
TEST_F(CtcClockTest, EachPeriodIsMetWithinAStep) {
  ASSERT_EQ(timer().Begin(), Error::kOk);
  // The prescaler is in TCCR2B while the timer runs.
  ASSERT_EQ(timer().Start(), Error::kOk);
  constexpr std::array<uint32_t, 7> kPeriods{2, 3, 10, 100, 1000, 10000, 13107};

  for (uint32_t const period : kPeriods) {
    ASSERT_EQ(timer().SetPeriod(static_cast<TimeType>(period)), Error::kOk)
        << period << " us";
    // Running, the period waits for the match.
    timer().OnInterrupt();
    uint64_t const wanted{period * kClock / kMicrosecondsPerSecond};

    EXPECT_LE(CtcPeriodInClocks(), wanted) << period << " us";
    EXPECT_LT(wanted - CtcPeriodInClocks(), Prescaler()) << period << " us";
  }
}

TEST_F(CtcClockTest, APeriodOfOneStepIsRefused) {
  // At no prescaler, 1 us is two clocks from 2 MHz on: a top of 1.
  constexpr bool kOneMicrosecondIsTwoSteps{2 * kMicrosecondsPerSecond <=
                                           kClock};

  EXPECT_EQ(timer().Begin(1U),
            kOneMicrosecondIsTwoSteps ? Error::kOk : Error::kPeriodOutOfRange);
}

TEST_F(CtcClockTest, ReadIsTheTimeOfTheStepsCounted) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(),
            50 * Prescaler() * kMicrosecondsPerSecond / kClock);
}

// Timer1 in CTC counts 65536 steps at the most of the largest prescaler.
constexpr uint64_t kLongestCtcPeriodOfTimer1InClocks{uint64_t{65536} * 1024};

class Timer1CtcClockTest : public ::testing::TestWithParam<uint32_t> {
 protected:
  void SetUp() override {
    TCCR1A = 0;
    TCCR1B = 0;
    ICR1 = 0;
    TCNT1.Set(0, true);
  }

  Ctc<Timer1Traits>& timer() { return timer_; }

 private:
  Ctc<Timer1Traits> timer_;
};

TEST_F(Timer1CtcClockTest, TheLongestPeriodIsTheLastThatFits) {
  uint64_t const longest{
      (kLongestCtcPeriodOfTimer1InClocks * kMicrosecondsPerSecond) / kClock};

  EXPECT_EQ(Ctc<Timer1Traits>::kPeriodMax, longest);
}

TEST_F(Timer1CtcClockTest, TheLongestPeriodIsTakenAndOneMoreRefused) {
  ASSERT_EQ(timer().Begin(Ctc<Timer1Traits>::kPeriodMax), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(PrescalerOfTimer1(), 1024U);

  EXPECT_EQ(timer().SetPeriod(Ctc<Timer1Traits>::kPeriodMax + 1U),
            Error::kPeriodOutOfRange);
}

// Each period is met to less than a step of the counter.
TEST_P(Timer1CtcClockTest, IsMetWithinAStep) {
  ASSERT_EQ(timer().Begin(GetParam()), Error::kOk);

  // The prescaler is in TCCR1B while the timer runs.
  ASSERT_EQ(timer().Start(), Error::kOk);

  uint64_t const wanted{GetParam() * kClock / kMicrosecondsPerSecond};
  uint64_t const period{(uint64_t{ICR1} + 1) * PrescalerOfTimer1()};
  EXPECT_LE(period, wanted);
  EXPECT_LT(wanted - period, PrescalerOfTimer1());
}

// From two clocks at 1 MHz to the longest period at 20 MHz, about 3.4 s.
INSTANTIATE_TEST_SUITE_P(FromTwoClocksToSeconds, Timer1CtcClockTest,
                         ::testing::Values(2U, 3U, 10U, 1000U, 100000U,
                                           1000000U, 3355443U),
                         [](::testing::TestParamInfo<uint32_t> const& period) {
                           return std::to_string(period.param) + "us";
                         });

}  // namespace
}  // namespace atmega_timer
