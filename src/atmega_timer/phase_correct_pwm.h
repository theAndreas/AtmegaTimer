// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief PhaseCorrectPwm, a periodic interrupt, a PWM and a time readout on a
 *        Timer/Counter of the AVR in phase correct PWM.
 *
 * The counter runs up to the top and down again: the period sets the top and
 * the prescaler; the overflow at the bottom calls the callback; and each
 * channel gives a PWM, at a duty that is a share of the top.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_PHASE_CORRECT_PWM_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_PHASE_CORRECT_PWM_H_

#include <Arduino.h>
#include <stdint.h>

#include "atmega_timer/Utility/array.h"
#include "atmega_timer/attributes.h"
#include "atmega_timer/core.h"
#include "atmega_timer/status.h"

namespace atmega_timer {

/*!
 * \brief The largest duty of a PWM, which keeps the output high; 0 keeps it
 *        low. The same on every timer, so that a sketch moves to another one
 *        without changing its duties.
 */
constexpr uint16_t kDutyMax{1023U};

namespace internal {

/*! \cond */
// The duty is a share of kDutyMax of the top, rounded.
constexpr uint32_t kDutyRounding{kDutyMax / 2U};

// The clocks of the longest period: every value of the counter up and down,
// at the largest prescaler. In 64 bits, as a 16-bit counter at 1024 times
// 1000 does not fit 32; only the compiler computes it.
template <typename Counter>
constexpr uint64_t LongestPeriodInClocks(uint8_t shift_largest) {
  return (uint64_t{CounterValues<Counter>()} << shift_largest) * 2U;
}

// Half of clocks_of_ms + clocks_of_us / 1000, rounded down: clocks_of_ms the
// clocks of the whole milliseconds, clocks_of_us those of the microseconds
// left, times 1000. Half of the odd clock of clocks_of_ms is 500 times 1000.
constexpr uint32_t HalfOf(uint32_t clocks_of_ms, uint32_t clocks_of_us) {
  return (clocks_of_ms / 2U) +
         (((kMicrosecondsPerMillisecond * (clocks_of_ms % 2U)) + clocks_of_us) /
          (2U * kMicrosecondsPerMillisecond));
}

// The clocks of half a period, rounded down. Its milliseconds and its
// microseconds apart, so that neither product leaves 32 bits up to the
// longest period of a 16-bit timer, about 134 s at 1 MHz.
constexpr uint32_t HalfPeriodInClocks(uint32_t period) {
  return HalfOf(kClocksPerMillisecond * (period / kMicrosecondsPerMillisecond),
                kClocksPerMillisecond * (period % kMicrosecondsPerMillisecond));
}
/*! \endcond */

}  // namespace internal

/*!
 * \brief A periodic interrupt, a PWM on each channel the mode leaves free, and
 *        a time readout on a Timer/Counter in phase correct PWM: mode 8 on
 *        Timer1, mode 5 on Timer2.
 *
 * What it shares with every mode, Begin(), Start() and the rest, comes from
 * internal::Core. A sketch uses the one that timer1.h or timer2.h gives,
 * timer1 or timer2.
 *
 * \tparam Traits the Timer/Counter, as Timer2Traits gives it, with
 *                `PhaseCorrectPwmMode`, as internal::Core takes it as `Mode`
 */
template <typename Traits>
class PhaseCorrectPwm
    : public internal::Core<PhaseCorrectPwm<Traits>, Traits,
                            typename Traits::PhaseCorrectPwmMode> {
  using Base = internal::Core<PhaseCorrectPwm<Traits>, Traits,
                              typename Traits::PhaseCorrectPwmMode>;
  // Base takes kPeriodMax, TopOf() and ApplyChannels() from here.
  friend Base;

 public:
  /*! \brief A time in microseconds, long enough for kPeriodMax. */
  using TimeType = typename Base::TimeType;

  /*!
   * \brief The longest period, in microseconds: the last whose half fits every
   *        step of the counter at the largest prescaler. At 16 MHz 32767 on
   *        Timer2, 8388607 on Timer1.
   */
  static constexpr TimeType kPeriodMax{static_cast<TimeType>(
      ((internal::LongestPeriodInClocks<typename Traits::Counter>(
            Traits::kShiftLargest) *
        internal::kMicrosecondsPerMillisecond) -
       1U) /
      internal::kClocksPerMillisecond)};

  /*!
   * \brief Starts the PWM of a channel on its pin, with a duty: channel B of
   *        Timer2 on pin 3.
   *
   * It breaks analogWrite() on that pin. Each channel has a duty of its own.
   *
   * \code{.cpp}
   * atmega_timer::Discard(
   *     atmega_timer::timer2.EnablePwm<atmega_timer::Channel::kB>(512));
   * \endcode
   *
   * \tparam Which the channel; one the timer does not have does not compile
   * \param duty the duty, 0 for always low to kDutyMax for always high
   * \return Error::kNotBegun, or Error::kDutyOutOfRange
   */
  template <Channel Which>
  ATMEGATIMER_NODISCARD Status EnablePwm(uint16_t duty) {
    static_assert(Base::HasChannel(Which), "the timer has no such channel");
    if (this->GetState() == State::kNotBegun) {
      return Error::kNotBegun;
    }
    if (duty > kDutyMax) {
      return Error::kDutyOutOfRange;
    }
    pinMode(CompareOutputOf<Which>::kPin, OUTPUT);
    Base::SetBit(Traits::Tccra(), CompareOutputOf<Which>::kCompareOutputBit);
    return SetPwmDuty<Which>(duty);
  }

  /*!
   * \brief Sets the duty of the PWM of a channel, which EnablePwm() started or
   *        is to start.
   * \tparam Which the channel; one the timer does not have does not compile
   * \param duty the duty, 0 for always low to kDutyMax for always high
   * \return Error::kNotBegun, or Error::kDutyOutOfRange
   */
  template <Channel Which>
  ATMEGATIMER_NODISCARD Status SetPwmDuty(uint16_t duty) {
    static_assert(Base::HasChannel(Which), "the timer has no such channel");
    if (this->GetState() == State::kNotBegun) {
      return Error::kNotBegun;
    }
    if (duty > kDutyMax) {
      return Error::kDutyOutOfRange;
    }
    duties_[Base::IndexOf(Which)] = duty;
    Base::Store(CompareOutputOf<Which>::Compare(), CompareValue(duty));
    return Error::kOk;
  }

  /*!
   * \brief Stops the PWM of a channel; the pin is a plain output again, at the
   *        level of its bit in its PORT register.
   * \tparam Which the channel; one the timer does not have does not compile
   */
  template <Channel Which>
  void DisablePwm() {
    static_assert(Base::HasChannel(Which), "the timer has no such channel");
    Base::ClearBit(Traits::Tccra(), CompareOutputOf<Which>::kCompareOutputBit);
  }

  /*!
   * \brief The time since the counter was last at its bottom, in steps of the
   *        prescaled clock.
   *
   * Waits for one step of the counter, at most 1024 clocks, to see whether it
   * counts up or down.
   *
   * \return the time in microseconds, or Error::kNotRunning
   */
  ATMEGATIMER_NODISCARD Result<TimeType> Read() {
    if (this->GetState() != State::kRunning) {
      return Error::kNotRunning;
    }
    // Up and down, the steps of a 16-bit counter take 17 bits.
    uint32_t steps{Base::Load(Traits::Tcnt())};
    Counter next{Base::Load(Traits::Tcnt())};
    while (next == steps) {
      next = Base::Load(Traits::Tcnt());
    }
    // Counting down: up to the top, and back to where it is.
    if (next < steps) {
      uint32_t const top{Base::Load(Traits::Top())};
      steps = (2U * top) - steps;
    }
    // The time is below kPeriodMax, which TimeType holds.
    uint32_t const clocks{steps << Traits::ShiftOf(this->clock_select())};
    return static_cast<TimeType>(internal::MicrosecondsOf(clocks));
  }

 private:
  using ClockSelect = typename Base::ClockSelect;
  using Counter = typename Base::Counter;
  using Mode = typename Traits::PhaseCorrectPwmMode;

  template <Channel Which>
  using CompareOutputOf = typename Base::template CompareOutputOf<Which>;

  template <uint8_t Index>
  using ChannelIndex = typename Base::template ChannelIndex<Index>;

  // The top for a period, and in clock_select the smallest prescaler that lets
  // it fit the counter; 0 for a period too short for one step, or too long.
  // The counter runs up to the top and down again, so half the period is
  // counted.
  static Counter TopOf(TimeType period, ClockSelect& clock_select) {
    uint32_t const half{internal::HalfPeriodInClocks(period)};
    for (uint8_t select{static_cast<uint8_t>(ClockSelect::kDivide1)};
         select <= static_cast<uint8_t>(Traits::kClockSelectLargest);
         ++select) {
      uint32_t const top{half >>
                         Traits::ShiftOf(static_cast<ClockSelect>(select))};
      if (top < internal::CounterValues<Counter>()) {
        clock_select = static_cast<ClockSelect>(select);
        return static_cast<Counter>(top);
      }
    }
    clock_select = ClockSelect::kNoClock;
    return 0U;
  }

  // The compare value of a duty at the top, rounded: 0 is the bottom, which
  // keeps the output low, and kDutyMax the top, which keeps it high. In 32
  // bits: 255 * 1023 does not fit 16, 65535 * 1023 does 32.
  Counter CompareValue(uint16_t duty) const {
    uint32_t const product{static_cast<uint32_t>(this->top()) * duty};
    return static_cast<Counter>((product + internal::kDutyRounding) / kDutyMax);
  }

  // The duty of each channel at the top, for Base when the period changes.
  void ApplyChannels() { ApplyDuties(ChannelIndex<0U>{}); }

  // The duty of each channel at the top, from the one at Index on.
  template <uint8_t Index>
  void ApplyDuties(ChannelIndex<Index> /*index*/) {
    constexpr Channel kWhich{static_cast<Channel>(
        static_cast<uint8_t>(Mode::kFirstChannel) + Index)};
    CompareOutputOf<kWhich>::Compare() = CompareValue(duties_[Index]);
    ApplyDuties(ChannelIndex<static_cast<uint8_t>(Index + 1U)>{});
  }

  // Past the last channel: chosen over the template, as it is no template.
  void ApplyDuties(ChannelIndex<Mode::kChannelCount> /*end*/) {}

  // The duty SetPwmDuty() was given to each channel, which a new period
  // applies to its top.
  internal::array<uint16_t, Mode::kChannelCount> duties_{};
};

/*! \cond */
// The definition C++11 needs for a static constexpr member that is used by
// reference.
template <typename Traits>
constexpr typename PhaseCorrectPwm<Traits>::TimeType
    PhaseCorrectPwm<Traits>::kPeriodMax;
/*! \endcond */

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_PHASE_CORRECT_PWM_H_
