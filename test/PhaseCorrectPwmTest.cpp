// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// PhaseCorrectPwm on the registers of Timer2 in test/mock/avr/io.h, at the
// Uno's 16 MHz: each test makes a timer of its own, on registers set back to
// those after a reset.

#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <array>

#include "Arduino.h"
#include "atmega_timer/Bit.h"
#include "atmega_timer/core.h"
#include "atmega_timer/phase_correct_pwm.h"
#include "atmega_timer/status.h"
#include "atmega_timer/timer2.h"
#include "atmega_timer/timer2_traits.h"
#include "test/pins.h"
#include "test/printers.h"  // IWYU pragma: keep

namespace atmega_timer {
namespace {

using TimeType = Timer2::TimeType;

// The compare register of channel A of OtherTraits, which Timer2 lacks.
uint8_t volatile other_compare_a{0};

// Traits on the registers of Timer2 whose every other value differs from
// Timer2Traits, so that a test shows where the timer takes it from: other mode
// bits, the prescalers 1, 8, 64 and 256, two channels, A on pin 5 and COM2A1
// and B on pin 6 and COM2B0, pins of Timer2 on neither board, and the
// interrupt and its flag at bits 1 and 2.
struct OtherTraits {
  using Counter = uint8_t;
  using TimeType = Timer2Traits::TimeType;

  enum class ClockSelect : uint8_t {
    kNoClock = 0,
    kDivide1,
    kDivide8,
    kDivide64,
    kDivide256
  };
  static constexpr ClockSelect kClockSelectLargest{ClockSelect::kDivide256};
  static constexpr uint8_t kShiftLargest{8u};

  static uint8_t ShiftOf(ClockSelect clock_select) {
    switch (clock_select) {
      case ClockSelect::kDivide8:
        return 3u;
      case ClockSelect::kDivide64:
        return 6u;
      case ClockSelect::kDivide256:
        return kShiftLargest;
      case ClockSelect::kNoClock:
      case ClockSelect::kDivide1:
      default:
        return 0u;
    }
  }

  struct PhaseCorrectPwmMode {
    // WGM21 alone.
    static constexpr uint8_t kModeTccra{0x02u};
    static constexpr uint8_t kModeTccrb{0u};
    static constexpr bool kTopIsBuffered{true};
    static constexpr uint8_t kInterruptBit{1u};
    static constexpr uint8_t kFlagBit{2u};
    static constexpr Channel kFirstChannel{Channel::kA};
    static constexpr uint8_t kChannelCount{2u};
  };

  template <Channel Which>
  struct CompareOutput;

  static uint8_t volatile& Tccra() { return TCCR2A; }
  static uint8_t volatile& Tccrb() { return TCCR2B; }
  static uint8_t volatile& Top() { return OCR2A; }
  static uint8_t volatile& Timsk() { return TIMSK2; }
  static uint8_t volatile& Tifr() { return TIFR2; }
  static MockCounter<uint8_t>& Tcnt() { return TCNT2; }
};

template <>
struct OtherTraits::CompareOutput<Channel::kA> {
  static constexpr uint8_t kPin{5};
  static constexpr uint8_t kCompareOutputBit{COM2A1};
  static uint8_t volatile& Compare() { return other_compare_a; }
};

template <>
struct OtherTraits::CompareOutput<Channel::kB> {
  static constexpr uint8_t kPin{6};
  static constexpr uint8_t kCompareOutputBit{COM2B0};
  static uint8_t volatile& Compare() { return OCR2B; }
};

// The bits of TCCR2B that select the clock, and their values.
constexpr uint8_t kClockSelectMask{0x07};
constexpr uint8_t kDivide1{1};
constexpr uint8_t kDivide8{2};
constexpr uint8_t kDivide32{3};
constexpr uint8_t kDivide64{4};
constexpr uint8_t kDivide128{5};
constexpr uint8_t kDivide256{6};
constexpr uint8_t kDivide1024{7};

int overflow_calls{0};

void CountOverflow() { ++overflow_calls; }

bool BitIsSet(uint8_t reg, unsigned bit) {
  return Bit<uint8_t>::ReadBit(reg, static_cast<uint8_t>(bit));
}

uint8_t ClockSelect() {
  return Bit<uint8_t>::ReadBitGroup(TCCR2B, kClockSelectMask);
}

class PhaseCorrectPwmTest : public ::testing::Test {
 protected:
  void SetUp() override {
    TCCR2A = 0;
    TCCR2B = 0;
    OCR2A = 0;
    OCR2B = 0;
    TIMSK2 = 0;
    TIFR2 = 0;
    TCNT2.Set(0, true);
    other_compare_a = 0;
    mock_pin_mode.fill(INPUT);
    overflow_calls = 0;
  }

  Timer2& timer() { return timer_; }
  PhaseCorrectPwm<OtherTraits>& other_timer() { return other_timer_; }

 private:
  Timer2 timer_;
  PhaseCorrectPwm<OtherTraits> other_timer_;
};

TEST_F(PhaseCorrectPwmTest, BeginSetsPhaseCorrectPwmWithoutAClock) {
  TCCR2A = 0xFF;
  TCCR2B = 0xFF;

  EXPECT_EQ(timer().Begin(), Error::kOk);

  EXPECT_EQ(timer().GetState(), State::kIdle);
  EXPECT_TRUE(BitIsSet(TCCR2A, WGM20));
  EXPECT_FALSE(BitIsSet(TCCR2A, WGM21));
  EXPECT_TRUE(BitIsSet(TCCR2B, WGM22));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(PhaseCorrectPwmTest, BeginIsRefusedTheSecondTime) {
  ASSERT_EQ(timer().Begin(), Error::kOk);

  EXPECT_EQ(timer().Begin(), Error::kBegunAlready);
}

TEST_F(PhaseCorrectPwmTest,
       BeginRefusesAPeriodAboveTheLongestAndStaysNotBegun) {
  EXPECT_EQ(timer().Begin(static_cast<TimeType>(Timer2::kPeriodMax + 1u)),
            Error::kPeriodOutOfRange);

  EXPECT_EQ(timer().GetState(), State::kNotBegun);
}

TEST_F(PhaseCorrectPwmTest, BeginRefusesAPeriodOfZeroAndStaysNotBegun) {
  EXPECT_EQ(timer().Begin(0u, CountOverflow), Error::kPeriodOutOfRange);

  EXPECT_EQ(timer().GetState(), State::kNotBegun);
  EXPECT_EQ(timer().GetCallback(), nullptr);
  EXPECT_EQ(timer().Start(), Error::kNotBegun);
}

TEST_F(PhaseCorrectPwmTest, TheLongestPeriodIsAbout33Milliseconds) {
  // 256 steps up and down at 16 MHz / 1024.
  EXPECT_EQ(Timer2::kPeriodMax, 32767u);
}

// The period is 2 * OCR2A * prescaler / 16 MHz: up to OCR2A and down again.
TEST_F(PhaseCorrectPwmTest, APeriodSetsTheTopAndTheSmallestPrescaler) {
  ASSERT_EQ(timer().Begin(10u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(OCR2A, 80);
  EXPECT_EQ(ClockSelect(), kDivide1);

  struct Case {
    TimeType period;
    uint8_t top;
    uint8_t clock_select;
  };
  std::array<Case, 6> const cases{{{100u, 100, kDivide8},
                                   {1000u, 250, kDivide32},
                                   {2000u, 250, kDivide64},
                                   {4000u, 250, kDivide128},
                                   {8000u, 250, kDivide256},
                                   {32767u, 255, kDivide1024}}};
  for (auto const& c : cases) {
    EXPECT_EQ(timer().SetPeriod(c.period), Error::kOk) << c.period << " us";
    EXPECT_EQ(OCR2A, c.top) << c.period << " us";
    EXPECT_EQ(ClockSelect(), c.clock_select) << c.period << " us";
  }
}

TEST_F(PhaseCorrectPwmTest, SetPeriodIsRefusedBeforeBegin) {
  EXPECT_EQ(timer().SetPeriod(1000u), Error::kNotBegun);

  EXPECT_EQ(OCR2A, 0);
}

TEST_F(PhaseCorrectPwmTest, SetPeriodRefusesAPeriodAboveTheLongest) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(timer().SetPeriod(static_cast<TimeType>(Timer2::kPeriodMax + 1u)),
            Error::kPeriodOutOfRange);
  EXPECT_EQ(OCR2A, 250);
}

TEST_F(PhaseCorrectPwmTest, SetPeriodRefusesAPeriodOfZero) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().SetPeriod(0u), Error::kPeriodOutOfRange);

  EXPECT_EQ(OCR2A, 250);
  EXPECT_EQ(ClockSelect(), kDivide32);
}

TEST_F(PhaseCorrectPwmTest, SetPeriodKeepsTheShareOfTheDuty) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().EnablePwm<Channel::kB>(kDutyMax), Error::kOk);

  ASSERT_EQ(timer().SetPeriod(100u), Error::kOk);

  EXPECT_EQ(OCR2A, 100);
  EXPECT_EQ(OCR2B, 100);
}

TEST_F(PhaseCorrectPwmTest, StartSelectsTheClockFromZero) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  TCNT2.Set(42, false);

  EXPECT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().GetState(), State::kRunning);
  EXPECT_EQ(ClockSelect(), kDivide32);
  EXPECT_EQ(static_cast<uint8_t>(TCNT2), 0);
}

TEST_F(PhaseCorrectPwmTest, StartWithoutCallbackLeavesTheInterruptOff) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(PhaseCorrectPwmTest, StartWithCallbackEnablesTheInterrupt) {
  ASSERT_EQ(timer().Begin(1000u, CountOverflow), Error::kOk);
  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK2, TOIE2));
}

// The core's init() runs Timer2 before setup(), so its overflow flag is set:
// enabled with it, the interrupt would come at once.
TEST_F(PhaseCorrectPwmTest,
       StartClearsTheOverflowFlagBeforeItEnablesTheInterrupt) {
  ASSERT_EQ(timer().Begin(1000u, CountOverflow), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  // Written with a 1, which clears it.
  EXPECT_TRUE(BitIsSet(TIFR2, TOV2));
}

TEST_F(PhaseCorrectPwmTest, AttachInterruptWhileRunningClearsTheOverflowFlag) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_FALSE(BitIsSet(TIFR2, TOV2));

  ASSERT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIFR2, TOV2));
}

TEST_F(PhaseCorrectPwmTest, StartIsRefusedBeforeBegin) {
  EXPECT_EQ(timer().Start(), Error::kNotBegun);

  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(PhaseCorrectPwmTest, StartIsRefusedWhileRunning) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().Start(), Error::kRunning);
}

TEST_F(PhaseCorrectPwmTest, StartAfterStopBeginsFromZero) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);
  TCNT2.Set(42, true);

  EXPECT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(static_cast<uint8_t>(TCNT2), 0);
}

TEST_F(PhaseCorrectPwmTest, TheOverflowCallsTheCallback) {
  ASSERT_EQ(timer().Begin(1000u, CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  timer().OnInterrupt();
  timer().OnInterrupt();

  EXPECT_EQ(overflow_calls, 2);
}

TEST_F(PhaseCorrectPwmTest, TheOverflowWithoutCallbackCallsNothing) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  timer().OnInterrupt();

  EXPECT_EQ(overflow_calls, 0);
}

TEST_F(PhaseCorrectPwmTest, StopDeselectsTheClock) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().Stop(), Error::kOk);

  EXPECT_EQ(timer().GetState(), State::kStopped);
  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(PhaseCorrectPwmTest, StopIsRefusedUnlessRunning) {
  EXPECT_EQ(timer().Stop(), Error::kNotRunning);
  EXPECT_EQ(timer().GetState(), State::kNotBegun);

  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  EXPECT_EQ(timer().Stop(), Error::kNotRunning);
  EXPECT_EQ(timer().GetState(), State::kIdle);
}

TEST_F(PhaseCorrectPwmTest, ResumeLetsTheTimerRunAgain) {
  ASSERT_EQ(timer().Begin(100u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);

  EXPECT_EQ(timer().Resume(), Error::kOk);

  EXPECT_EQ(timer().GetState(), State::kRunning);
  EXPECT_EQ(ClockSelect(), kDivide8);
  EXPECT_TRUE(timer().Read().IsOk());
}

TEST_F(PhaseCorrectPwmTest, ResumeIsRefusedUnlessStopped) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(timer().Resume(), Error::kNotStopped);

  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(timer().Resume(), Error::kNotStopped);
}

TEST_F(PhaseCorrectPwmTest, EnablePwmDrivesOc2bWithTheDutyOfTheTop) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(timer().EnablePwm<Channel::kB>(512), Error::kOk);

  EXPECT_EQ(mock_pin_mode[kOc2bPin], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B0));
  // 512 / 1023 of the top, 250, rounded.
  EXPECT_EQ(OCR2B, 125);
}

TEST_F(PhaseCorrectPwmTest, EnablePwmIsRefusedBeforeBegin) {
  EXPECT_EQ(timer().EnablePwm<Channel::kB>(512), Error::kNotBegun);

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_EQ(mock_pin_mode[kOc2bPin], INPUT);
}

TEST_F(PhaseCorrectPwmTest, AFullDutyIsTheTopAndNoDutyTheBottom) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  // OCR2B at the top keeps the output high all the time, at the bottom low.
  ASSERT_EQ(timer().EnablePwm<Channel::kB>(kDutyMax), Error::kOk);
  EXPECT_EQ(OCR2B, OCR2A);
  ASSERT_EQ(timer().SetPwmDuty<Channel::kB>(0), Error::kOk);
  EXPECT_EQ(OCR2B, 0);
}

TEST_F(PhaseCorrectPwmTest, SetPwmDutySetsTheCompareValue) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().EnablePwm<Channel::kB>(512), Error::kOk);

  EXPECT_EQ(timer().SetPwmDuty<Channel::kB>(256), Error::kOk);

  // 256 / 1023 of the top, 250, rounded.
  EXPECT_EQ(OCR2B, 63);
}

TEST_F(PhaseCorrectPwmTest, TheCompareValueIsComputedIn32Bits) {
  ASSERT_EQ(timer().Begin(Timer2::kPeriodMax), Error::kOk);
  ASSERT_EQ(OCR2A, 255);

  ASSERT_EQ(timer().EnablePwm<Channel::kB>(600), Error::kOk);

  // 255 * 600 does not fit 16 bits, which would give 21.
  EXPECT_EQ(OCR2B, 150);
}

TEST_F(PhaseCorrectPwmTest, ADutyAboveTheMaxIsRefused) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().EnablePwm<Channel::kB>(512), Error::kOk);

  EXPECT_EQ(timer().SetPwmDuty<Channel::kB>(kDutyMax + 1u),
            Error::kDutyOutOfRange);

  EXPECT_EQ(OCR2B, 125);
}

TEST_F(PhaseCorrectPwmTest, EnablePwmWithADutyAboveTheMaxChangesNothing) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(timer().EnablePwm<Channel::kB>(kDutyMax + 1u),
            Error::kDutyOutOfRange);

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_EQ(mock_pin_mode[kOc2bPin], INPUT);
}

TEST_F(PhaseCorrectPwmTest, SetPwmDutyIsRefusedBeforeBegin) {
  OCR2B = 7;

  EXPECT_EQ(timer().SetPwmDuty<Channel::kB>(256), Error::kNotBegun);

  EXPECT_EQ(OCR2B, 7);
}

TEST_F(PhaseCorrectPwmTest, DisablePwmDisconnectsOc2b) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().EnablePwm<Channel::kB>(512), Error::kOk);

  timer().DisablePwm<Channel::kB>();

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
}

TEST_F(PhaseCorrectPwmTest, AttachInterruptRefusesNullptr) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(timer().AttachInterrupt(nullptr), Error::kNoCallback);

  EXPECT_EQ(timer().GetCallback(), nullptr);
}

TEST_F(PhaseCorrectPwmTest, AttachInterruptWhileRunningEnablesTheInterrupt) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK2, TOIE2));
  timer().OnInterrupt();
  EXPECT_EQ(overflow_calls, 1);
}

TEST_F(PhaseCorrectPwmTest, AttachInterruptBeforeStartLeavesTheInterruptOff) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
  EXPECT_EQ(timer().GetCallback(), &CountOverflow);
}

// A-10: the interrupt stayed off, and the callback was never called.
TEST_F(PhaseCorrectPwmTest,
       AttachInterruptWhileStoppedCallsTheCallbackAfterResume) {
  ASSERT_EQ(timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);

  ASSERT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Resume(), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(PhaseCorrectPwmTest,
       DetachInterruptDisablesTheInterruptAndForgetsTheCallback) {
  ASSERT_EQ(timer().Begin(1000u, CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  timer().DetachInterrupt();

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
  EXPECT_EQ(timer().GetCallback(), nullptr);
}

TEST_F(PhaseCorrectPwmTest, StartAfterDetachInterruptLeavesTheInterruptOff) {
  ASSERT_EQ(timer().Begin(1000u, CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  timer().DetachInterrupt();
  ASSERT_EQ(timer().Stop(), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(PhaseCorrectPwmTest, ReadIsRefusedUnlessRunning) {
  ASSERT_EQ(timer().Begin(100u), Error::kOk);

  Result<TimeType> const result{timer().Read()};

  EXPECT_EQ(result.GetError(), Error::kNotRunning);
  EXPECT_EQ(result.GetValue(), 0u);
}

// At 100 us the prescaler is 8: a step of the counter is 0.5 us.
TEST_F(PhaseCorrectPwmTest, ReadCountsTheStepsUpwards) {
  ASSERT_EQ(timer().Begin(100u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 25u);
}

TEST_F(PhaseCorrectPwmTest, ReadCountsTheStepsBackDownFromTheTop) {
  ASSERT_EQ(timer().Begin(100u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  // 100 steps up and 92 down.
  TCNT2.Set(8, false);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 96u);
}

TEST_F(PhaseCorrectPwmTest, ReadWaitsForTheCounterToStep) {
  ASSERT_EQ(timer().Begin(100u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, false);
  // The prescaler of 8 lets the CPU read each value several times.
  TCNT2.SetReadsPerStep(4);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  // 100 steps up and 50 down, of 0.5 us.
  EXPECT_EQ(result.GetValue(), 75u);
}

// 16 steps of the counter are as many microseconds as the prescaler divides
// by: 16 MHz / 16.
TEST_F(PhaseCorrectPwmTest, ReadScalesByEachPrescaler) {
  ASSERT_EQ(timer().Begin(10u), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  struct Case {
    TimeType period;
    TimeType sixteen_steps;
  };
  std::array<Case, 7> const cases{{{10u, 1u},
                                   {100u, 8u},
                                   {1000u, 32u},
                                   {2000u, 64u},
                                   {4000u, 128u},
                                   {8000u, 256u},
                                   {32767u, 1024u}}};

  for (auto const& c : cases) {
    ASSERT_EQ(timer().SetPeriod(c.period), Error::kOk);
    TCNT2.Set(16, true);

    Result<TimeType> const result{timer().Read()};

    ASSERT_TRUE(result.IsOk()) << "period " << c.period;
    EXPECT_EQ(result.GetValue(), c.sixteen_steps) << "period " << c.period;
  }
}

TEST_F(PhaseCorrectPwmTest, BeginSetsTheModeOfTheTraits) {
  TCCR2A = 0xFF;
  TCCR2B = 0xFF;

  EXPECT_EQ(other_timer().Begin(), Error::kOk);

  // WGM21 alone, and no clock.
  EXPECT_EQ(TCCR2A, 0x02);
  EXPECT_EQ(TCCR2B, 0x00);
}

TEST_F(PhaseCorrectPwmTest, APeriodTakesThePrescalersOfTheTraits) {
  ASSERT_EQ(other_timer().Begin(1000u), Error::kOk);

  ASSERT_EQ(other_timer().Start(), Error::kOk);

  // 8000 clocks up, which 32 of Timer2 would make 250 steps; here 64 makes
  // 125, selected by the third value.
  EXPECT_EQ(OCR2A, 125);
  EXPECT_EQ(ClockSelect(), 3);
}

TEST_F(PhaseCorrectPwmTest,
       TheLongestPeriodIsThatOfTheLargestPrescalerOfTheTraits) {
  // 256 steps up and down at 16 MHz / 256.
  EXPECT_EQ(PhaseCorrectPwm<OtherTraits>::kPeriodMax, 8191u);
}

TEST_F(PhaseCorrectPwmTest, EnablePwmDrivesThePinAndTheBitOfTheTraits) {
  ASSERT_EQ(other_timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(other_timer().EnablePwm<Channel::kB>(512), Error::kOk);

  EXPECT_EQ(mock_pin_mode[6], OUTPUT);
  EXPECT_EQ(mock_pin_mode[kOc2bPin], INPUT);
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2B0));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
  // 512 / 1023 of the top, 125, rounded.
  EXPECT_EQ(OCR2B, 63);
}

TEST_F(PhaseCorrectPwmTest, EnablePwmOfChannelADrivesItsOwnPinAndBit) {
  ASSERT_EQ(other_timer().Begin(1000u), Error::kOk);

  EXPECT_EQ(other_timer().EnablePwm<Channel::kA>(512), Error::kOk);

  EXPECT_EQ(mock_pin_mode[5], OUTPUT);
  EXPECT_EQ(mock_pin_mode[6], INPUT);
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2A1));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B0));
  // 512 / 1023 of the top, 125, rounded.
  EXPECT_EQ(other_compare_a, 63);
}

TEST_F(PhaseCorrectPwmTest, EachChannelKeepsADutyOfItsOwn) {
  ASSERT_EQ(other_timer().Begin(1000u), Error::kOk);

  ASSERT_EQ(other_timer().EnablePwm<Channel::kA>(512), Error::kOk);
  ASSERT_EQ(other_timer().EnablePwm<Channel::kB>(256), Error::kOk);

  // 512 and 256 / 1023 of the top, 125, rounded.
  EXPECT_EQ(other_compare_a, 63);
  EXPECT_EQ(OCR2B, 31);
}

TEST_F(PhaseCorrectPwmTest, SetPeriodKeepsTheShareOfTheDutyOfEachChannel) {
  ASSERT_EQ(other_timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(other_timer().EnablePwm<Channel::kA>(512), Error::kOk);
  ASSERT_EQ(other_timer().EnablePwm<Channel::kB>(256), Error::kOk);

  ASSERT_EQ(other_timer().SetPeriod(2000u), Error::kOk);

  // 512 and 256 / 1023 of the new top, 250, rounded.
  ASSERT_EQ(OCR2A, 250);
  EXPECT_EQ(other_compare_a, 125);
  EXPECT_EQ(OCR2B, 63);
}

TEST_F(PhaseCorrectPwmTest, DisablePwmLeavesTheOtherChannel) {
  ASSERT_EQ(other_timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(other_timer().EnablePwm<Channel::kA>(512), Error::kOk);
  ASSERT_EQ(other_timer().EnablePwm<Channel::kB>(256), Error::kOk);

  other_timer().DisablePwm<Channel::kA>();

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2A1));
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2B0));
}

TEST_F(PhaseCorrectPwmTest, DisablePwmClearsTheBitOfTheTraits) {
  ASSERT_EQ(other_timer().Begin(1000u), Error::kOk);
  ASSERT_EQ(other_timer().EnablePwm<Channel::kB>(512), Error::kOk);

  other_timer().DisablePwm<Channel::kB>();

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B0));
}

TEST_F(PhaseCorrectPwmTest, StartEnablesTheInterruptOfTheTraits) {
  ASSERT_EQ(other_timer().Begin(1000u, CountOverflow), Error::kOk);

  ASSERT_EQ(other_timer().Start(), Error::kOk);

  EXPECT_EQ(TIMSK2, 0x02);
  EXPECT_EQ(TIFR2, 0x04);
}

TEST_F(PhaseCorrectPwmTest, DetachInterruptDisablesTheInterruptOfTheTraits) {
  ASSERT_EQ(other_timer().Begin(1000u, CountOverflow), Error::kOk);
  ASSERT_EQ(other_timer().Start(), Error::kOk);

  other_timer().DetachInterrupt();

  EXPECT_EQ(TIMSK2, 0x00);
}

}  // namespace
}  // namespace atmega_timer
