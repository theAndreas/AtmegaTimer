// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Ctc, a periodic interrupt, a toggle on each channel and a time readout
 *        on a Timer/Counter of the AVR in CTC.
 *
 * The counter runs from 0 up to the top and starts at 0 again: the period sets
 * the top and the prescaler; the match at the top calls the callback; and each
 * channel toggles its pin once a period.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_CTC_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_CTC_H_

#include <Arduino.h>
#include <stdint.h>

#include "atmega_timer/attributes.h"
#include "atmega_timer/core.h"
#include "atmega_timer/status.h"

namespace atmega_timer {

namespace internal {

/*! \cond */
// The clocks of the longest period in CTC: every value of the counter once,
// at the largest prescaler. In 64 bits, as a 16-bit counter at 1024 times
// 1000 does not fit 32; only the compiler computes it.
template <typename Counter>
constexpr uint64_t LongestCtcPeriodInClocks(uint8_t shift_largest) {
  return uint64_t{CounterValues<Counter>()} << shift_largest;
}
/*! \endcond */

}  // namespace internal

/*!
 * \brief A periodic interrupt, a toggle on each channel and a time readout on a
 *        Timer/Counter in CTC: mode 12 on Timer1, mode 2 on Timer2.
 *
 * The counter counts the period once, rather than up and down as in
 * PhaseCorrectPwm: twice the resolution, half the longest period, and Read()
 * without a wait; but no PWM. What it shares with every mode, Begin(), Start()
 * and the rest, comes from internal::Core. A sketch uses the one that
 * timer1_ctc.h or timer2_ctc.h gives, timer1_ctc or timer2_ctc, and not timer1
 * or timer2 beside it: a sketch with both objects of one timer fails to link.
 *
 * \tparam Traits the Timer/Counter, as Timer2Traits gives it, with `CtcMode`,
 *                as internal::Core takes it as `Mode`, and its
 *                `kFirstOwnCompare`, the first channel whose compare value is
 *                not the top
 */
template <typename Traits>
class Ctc
    : public internal::Core<Ctc<Traits>, Traits, typename Traits::CtcMode> {
  using Base = internal::Core<Ctc<Traits>, Traits, typename Traits::CtcMode>;
  // Base takes kPeriodMax, kStepsMax, TopOf() and ApplyChannels() from here.
  friend Base;

 public:
  /*! \brief A time in microseconds, long enough for kPeriodMax. */
  using TimeType = typename Base::TimeType;

  /*!
   * \brief The longest period, in microseconds: every step of the counter once
   *        at the largest prescaler, rounded down. At 16 MHz 16384 on Timer2,
   *        4194304 on Timer1.
   */
  static constexpr TimeType kPeriodMax{static_cast<TimeType>(
      (internal::LongestCtcPeriodInClocks<typename Traits::Counter>(
           Traits::kShiftLargest) *
       internal::kMicrosecondsPerMillisecond) /
      internal::kClocksPerMillisecond)};

  /*!
   * \brief Toggles the pin of a channel once a period: channel B of Timer2 on
   *        pin 3.
   *
   * A square wave at half the frequency of the period, high for one period and
   * low for the next, made by the timer without the CPU. It breaks
   * analogWrite() on that pin. Channel A of Timer2 toggles at the top, as
   * OCR2A is the top; every other channel at 0, which the counter passes in
   * every period, whatever the period.
   *
   * \code{.cpp}
   * atmega_timer::Discard(
   *     atmega_timer::timer2_ctc.EnableToggle<atmega_timer::Channel::kB>());
   * \endcode
   *
   * \tparam Which the channel; one the timer does not have does not compile
   * \return Error::kNotBegun
   */
  template <Channel Which>
  ATMEGATIMER_NODISCARD Status EnableToggle() {
    static_assert(Base::HasChannel(Which), "the timer has no such channel");
    if (this->GetState() == State::kNotBegun) {
      return Error::kNotBegun;
    }
    MatchAtTheBottom<Which>(OwnCompare<HasACompareOfItsOwn(Which)>{});
    pinMode(CompareOutputOf<Which>::kPin, OUTPUT);
    Base::SetBit(Traits::Tccra(), CompareOutputOf<Which>::kToggleBit);
    return Error::kOk;
  }

  /*!
   * \brief Stops the toggle of a channel; the pin is a plain output again, at
   *        the level of its bit in its PORT register.
   * \tparam Which the channel; one the timer does not have does not compile
   */
  template <Channel Which>
  void DisableToggle() {
    static_assert(Base::HasChannel(Which), "the timer has no such channel");
    Base::ClearBit(Traits::Tccra(), CompareOutputOf<Which>::kToggleBit);
  }

  /*!
   * \brief The time since the counter was last at 0, in steps of the prescaled
   *        clock.
   * \return the time in microseconds, or Error::kNotRunning
   */
  ATMEGATIMER_NODISCARD Result<TimeType> Read() {
    if (this->GetState() != State::kRunning) {
      return Error::kNotRunning;
    }
    uint32_t const steps{Base::Load(Traits::Tcnt())};
    // The time is below kPeriodMax, which TimeType holds.
    uint32_t const clocks{steps << Traits::ShiftOf(this->clock_select())};
    return static_cast<TimeType>(internal::MicrosecondsOf(clocks));
  }

 private:
  using ClockSelect = typename Base::ClockSelect;
  using Counter = typename Base::Counter;
  using Mode = typename Traits::CtcMode;

  template <Channel Which>
  using CompareOutputOf = typename Base::template CompareOutputOf<Which>;

  // The most steps of a period, for Base: every value of the counter once.
  static constexpr uint32_t kStepsMax{internal::CounterValues<Counter>()};

  // Whether the compare value of a channel is its own, rather than the top.
  static constexpr bool HasACompareOfItsOwn(Channel which) {
    return static_cast<uint8_t>(which) >=
           static_cast<uint8_t>(Mode::kFirstOwnCompare);
  }

  // HasACompareOfItsOwn() as a type, to choose an overload when compiling: a
  // channel whose compare value is the top has no Compare() of its own.
  template <bool Own>
  struct OwnCompare {};

  // A channel's own compare value at 0, which the counter passes in every
  // period, whatever the period.
  template <Channel Which>
  static void MatchAtTheBottom(OwnCompare<true> /*own*/) {
    Base::Store(CompareOutputOf<Which>::Compare(), 0U);
  }

  // A channel whose compare value is the top matches there.
  template <Channel Which>
  static void MatchAtTheBottom(OwnCompare<false> /*own*/) {}

  // The top for a period, and in clock_select the smallest prescaler that lets
  // it fit the counter; 0 for a period too short for two steps, or too long.
  // The counter counts from 0 to the top, one step more than the top.
  static Counter TopOf(TimeType period, ClockSelect& clock_select) {
    uint32_t const clocks{internal::PeriodInClocks(period)};
    clock_select = Base::SmallestClockSelectFor(clocks);
    if (clock_select == ClockSelect::kNoClock) {
      return 0U;
    }
    uint32_t const steps{clocks >> Traits::ShiftOf(clock_select)};
    // No step at all: a period of 0, or of 1 us below 1 MHz.
    if (steps == 0U) {
      return 0U;
    }
    return static_cast<Counter>(steps - 1U);
  }

  // For Base when the period changes: a toggle matches at 0 or at the top,
  // which follow the period by themselves.
  void ApplyChannels() {}
};

/*! \cond */
// The definition C++11 needs for a static constexpr member that is used by
// reference.
template <typename Traits>
constexpr typename Ctc<Traits>::TimeType Ctc<Traits>::kPeriodMax;
/*! \endcond */

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_CTC_H_
