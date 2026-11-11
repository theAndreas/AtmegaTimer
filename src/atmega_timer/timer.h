// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer, a periodic interrupt, a PWM and a time readout on an 8-bit
 *        Timer/Counter of the AVR in phase correct PWM.
 *
 * The counter runs up to the top and down again: the period sets the top and
 * the prescaler; the overflow at the bottom calls the callback; and a channel
 * gives the PWM, at a duty that is a share of the top. What differs from one
 * Timer/Counter to another is in its traits, a parameter of the template:
 * timer2.h hands in those of Timer/Counter2 of the ATmega328P.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER_H_

#include <Arduino.h>
#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/Utility/array.h"
#include "atmega_timer/attributes.h"
#include "atmega_timer/status.h"

namespace atmega_timer {

/*!
 * \brief The largest duty of a PWM, which keeps the output high; 0 keeps it
 *        low. The same on every timer, so that a sketch moves to another one
 *        without changing its duties.
 */
constexpr uint16_t kDutyMax{1023U};

/*!
 * \brief A channel of a timer: its compare output, which gives a PWM on a pin
 *        of its own.
 */
enum class Channel : uint8_t {
  kA, /*!< OCxA. */
  kB  /*!< OCxB, after OCxA: the traits count the channels in this order. */
};

/*! \brief The function the overflow interrupt calls. */
using Callback = void (*)();

/*! \brief Where a timer is in its life. */
enum class State : uint8_t {
  kNotBegun, /*!< Begin() comes first. */
  kIdle,     /*!< Begun, not started. */
  kRunning,  /*!< Counting. */
  kStopped   /*!< Stopped by Stop(); Resume() goes on, Start() from 0. */
};

namespace internal {

/*! \cond */
// The clock select bits CSx2..CSx0, at the same place on every Timer/Counter.
constexpr uint8_t kClockSelectMask{0x07U};

// The counter has 256 values.
constexpr uint32_t kCounterValues{256U};
constexpr uint32_t kClocksPerMillisecond{F_CPU / 1000UL};
constexpr uint32_t kMicrosecondsPerMillisecond{1000U};
// The duty is a share of kDutyMax of the top, rounded.
constexpr uint32_t kDutyRounding{kDutyMax / 2U};

/*! \endcond */

}  // namespace internal

/*!
 * \brief A periodic interrupt, a PWM and a time readout on an 8-bit
 *        Timer/Counter in phase correct PWM, mode 5.
 *
 * Begin() first, then Start(). A function that can refuse returns a Status,
 * and leaves the timer as it was when it refuses. A sketch uses the one that
 * timer2.h gives, timer2.
 *
 * \tparam Traits the Timer/Counter, as Timer2Traits gives it: `TimeType`;
 *                `ClockSelect`, its prescalers, `kNoClock` at 0 and `kDivide1`
 *                at 1, the others in the values after it up to
 *                `kClockSelectLargest`, with `kShiftLargest` and `ShiftOf()`;
 *                `kModeTccra` and `kModeTccrb`, the bits of the mode;
 *                `kOverflowInterruptBit` and `kOverflowFlagBit`;
 *                `kFirstChannel` and `kChannelCount`, the channels that give a
 *                PWM, one after the other in Channel, and for each
 *                `PwmChannel<Output>`, with `kPin`, `kCompareOutputBit` and
 *                `Compare()`; and `Tccra()`, `Tccrb()`, `Top()`, `Timsk()`,
 *                `Tifr()` and `Tcnt()`, which return its registers.
 */
template <typename Traits>
class Timer {
 public:
  /*! \brief A time in microseconds, long enough for kPeriodMax. */
  using TimeType = typename Traits::TimeType;

  /*!
   * \brief The longest period, in microseconds: the last whose half fits 256
   *        steps of the largest prescaler. 32767 on Timer2 at 16 MHz.
   */
  static constexpr TimeType kPeriodMax{static_cast<TimeType>(
      (((internal::kCounterValues << Traits::kShiftLargest) * 2U *
        internal::kMicrosecondsPerMillisecond) -
       1U) /
      internal::kClocksPerMillisecond)};

  /*! \brief The period Begin() sets if none is given: 1 ms. */
  static constexpr TimeType kDefaultPeriod{1000U};

  /*!
   * \brief Sets the timer up in its mode, with a period and a callback; it does
   *        not count before Start().
   * \param period the period in microseconds, up to kPeriodMax
   * \param callback what the overflow interrupt calls, or nullptr for none
   * \return Error::kBegunAlready, or Error::kPeriodOutOfRange as SetPeriod()
   */
  ATMEGATIMER_NODISCARD Status Begin(TimeType period = kDefaultPeriod,
                                     Callback callback = nullptr) {
    ClockSelect clock_select{ClockSelect::kNoClock};
    uint8_t const top{TopOf(period, clock_select)};
    if (state_ != State::kNotBegun) {
      return Error::kBegunAlready;
    }
    if ((period > kPeriodMax) || (top == 0U)) {
      return Error::kPeriodOutOfRange;
    }
    // No clock and no output yet.
    Traits::Tccra() = Traits::kModeTccra;
    Traits::Tccrb() = Traits::kModeTccrb;
    state_ = State::kIdle;
    ApplyPeriod(top, clock_select);
    if (callback != nullptr) {
      Discard(AttachInterrupt(callback));  // not nullptr, so it attaches
    }
    return Error::kOk;
  }

  /*!
   * \brief Sets the period of the interrupt and of the PWM; the duty of the PWM
   *        keeps its share of it.
   * \param period the period in microseconds, up to kPeriodMax
   * \return Error::kNotBegun, or Error::kPeriodOutOfRange for a period too
   *         long, or too short for one step of the counter: 0, or 1 below 2 MHz
   */
  ATMEGATIMER_NODISCARD Status SetPeriod(TimeType period) {
    ClockSelect clock_select{ClockSelect::kNoClock};
    uint8_t const top{TopOf(period, clock_select)};
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    // A top of 0 would stop the counter at 0.
    if ((period > kPeriodMax) || (top == 0U)) {
      return Error::kPeriodOutOfRange;
    }
    ApplyPeriod(top, clock_select);
    return Error::kOk;
  }

  /*!
   * \brief Starts the counter at 0, and the interrupt if a callback is set.
   * \return Error::kNotBegun, or Error::kRunning
   */
  ATMEGATIMER_NODISCARD Status Start() {
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    if (state_ == State::kRunning) {
      return Error::kRunning;
    }
    Traits::Tcnt() = 0U;
    SelectClock(clock_select_);
    if (callback_ != nullptr) {
      // Off the bottom first, as version 1 did; one step takes at most 1024
      // clocks, 64 us at 16 MHz.
      while (Traits::Tcnt() == 0U) {
      }
      EnableOverflowInterrupt();
    }
    state_ = State::kRunning;
    return Error::kOk;
  }

  /*!
   * \brief Stops the counter where it is.
   * \return Error::kNotRunning
   */
  ATMEGATIMER_NODISCARD Status Stop() {
    if (state_ != State::kRunning) {
      return Error::kNotRunning;
    }
    SelectClock(ClockSelect::kNoClock);
    state_ = State::kStopped;
    return Error::kOk;
  }

  /*!
   * \brief Lets the counter go on from where Stop() stopped it.
   * \return Error::kNotStopped
   */
  ATMEGATIMER_NODISCARD Status Resume() {
    if (state_ != State::kStopped) {
      return Error::kNotStopped;
    }
    SelectClock(clock_select_);
    state_ = State::kRunning;
    return Error::kOk;
  }

  /*!
   * \brief Sets the function the overflow interrupt calls, and enables the
   *        interrupt if the timer runs.
   * \param callback the function
   * \return Error::kNoCallback for nullptr
   */
  ATMEGATIMER_NODISCARD Status AttachInterrupt(Callback callback) {
    if (callback == nullptr) {
      return Error::kNoCallback;
    }
    // The AVR writes the pointer a byte at a time: the interrupt must not call
    // through half of the new one.
    ClearBit(Traits::Timsk(), Traits::kOverflowInterruptBit);
    callback_ = callback;
    if (state_ == State::kRunning) {
      EnableOverflowInterrupt();
    }
    return Error::kOk;
  }

  /*!
   * \brief Disables the overflow interrupt and forgets its callback, so that
   *        Start() does not enable it again.
   */
  void DetachInterrupt() {
    ClearBit(Traits::Timsk(), Traits::kOverflowInterruptBit);
    // After the interrupt is off, which calls through it.
    callback_ = nullptr;
  }

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
   * \tparam Output the channel; one the timer does not have does not compile
   * \param duty the duty, 0 for always low to kDutyMax for always high
   * \return Error::kNotBegun, or Error::kDutyOutOfRange
   */
  template <Channel Output>
  ATMEGATIMER_NODISCARD Status EnablePwm(uint16_t duty) {
    static_assert(HasChannel(Output), "the timer has no such channel");
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    if (duty > kDutyMax) {
      return Error::kDutyOutOfRange;
    }
    pinMode(PwmChannelOf<Output>::kPin, OUTPUT);
    SetBit(Traits::Tccra(), PwmChannelOf<Output>::kCompareOutputBit);
    return SetPwmDuty<Output>(duty);
  }

  /*!
   * \brief Sets the duty of the PWM of a channel, which EnablePwm() started or
   *        is to start.
   * \tparam Output the channel; one the timer does not have does not compile
   * \param duty the duty, 0 for always low to kDutyMax for always high
   * \return Error::kNotBegun, or Error::kDutyOutOfRange
   */
  template <Channel Output>
  ATMEGATIMER_NODISCARD Status SetPwmDuty(uint16_t duty) {
    static_assert(HasChannel(Output), "the timer has no such channel");
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    if (duty > kDutyMax) {
      return Error::kDutyOutOfRange;
    }
    duties_[IndexOf(Output)] = duty;
    PwmChannelOf<Output>::Compare() = CompareValue(duty);
    return Error::kOk;
  }

  /*!
   * \brief Stops the PWM of a channel; the pin is a plain output again, at the
   *        level of its bit in its PORT register.
   * \tparam Output the channel; one the timer does not have does not compile
   */
  template <Channel Output>
  void DisablePwm() {
    static_assert(HasChannel(Output), "the timer has no such channel");
    ClearBit(Traits::Tccra(), PwmChannelOf<Output>::kCompareOutputBit);
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
    if (state_ != State::kRunning) {
      return Error::kNotRunning;
    }
    uint16_t steps{Traits::Tcnt()};
    uint8_t next{Traits::Tcnt()};
    while (next == steps) {
      next = Traits::Tcnt();
    }
    // Counting down: up to the top, and back to where it is.
    if (next < steps) {
      steps = static_cast<uint16_t>((2U * Traits::Top()) - steps);
    }
    // In clocks, then in microseconds: multiplied before divided, so that no
    // step is lost. At most 2 * 255 steps of 1024 clocks, times 1000, fit 32
    // bits; the time is below kPeriodMax, which TimeType holds.
    uint32_t const clocks{static_cast<uint32_t>(steps)
                          << Traits::ShiftOf(clock_select_)};
    return static_cast<TimeType>(
        (clocks * internal::kMicrosecondsPerMillisecond) /
        internal::kClocksPerMillisecond);
  }

  /*! \brief Where the timer is in its life. */
  State GetState() const { return state_; }

  /*! \brief The function the overflow interrupt calls, or nullptr. */
  Callback GetCallback() const { return callback_; }

  /*!
   * \brief Calls the callback; for the overflow interrupt, which the source of
   *        each timer defines, as timer2.cpp.
   */
  void OnOverflow() {
    Callback const callback{callback_};
    if (callback != nullptr) {
      callback();
    }
  }

 private:
  using ClockSelect = typename Traits::ClockSelect;

  template <Channel Output>
  using PwmChannelOf = typename Traits::template PwmChannel<Output>;

  // A channel's place among those of the timer, and in duties_.
  static constexpr uint8_t IndexOf(Channel output) {
    return static_cast<uint8_t>(static_cast<uint8_t>(output) -
                                static_cast<uint8_t>(Traits::kFirstChannel));
  }

  // One before the first wraps around to 255, beyond the last as well.
  static constexpr bool HasChannel(Channel output) {
    return IndexOf(output) < Traits::kChannelCount;
  }

  // A channel by its place, to go through all of them when compiling: each
  // has a register of its own, which no index reaches at run time.
  template <uint8_t Index>
  struct ChannelIndex {};

  // The top for a period, and in clock_select the smallest prescaler that lets
  // it fit the counter; 0 for a period too short for one step, or too long.
  // The counter runs up to the top and down again, so half the period is
  // counted. Multiplied before divided, so that a clock below 2 MHz keeps its
  // cycles; up to kPeriodMax the product is below 2^29.
  static uint8_t TopOf(TimeType period, ClockSelect& clock_select) {
    uint32_t const half{(internal::kClocksPerMillisecond * period) /
                        (2U * internal::kMicrosecondsPerMillisecond)};
    for (uint8_t select{static_cast<uint8_t>(ClockSelect::kDivide1)};
         select <= static_cast<uint8_t>(Traits::kClockSelectLargest);
         ++select) {
      uint32_t const top{half >>
                         Traits::ShiftOf(static_cast<ClockSelect>(select))};
      if (top < internal::kCounterValues) {
        clock_select = static_cast<ClockSelect>(select);
        return static_cast<uint8_t>(top);
      }
    }
    clock_select = ClockSelect::kNoClock;
    return 0U;
  }

  // The compare value of a duty at the top, rounded: 0 is the bottom, which
  // keeps the output low, and kDutyMax the top, which keeps it high. In 32
  // bits: 255 * 1023 does not fit 16.
  static uint8_t CompareValue(uint16_t duty) {
    uint32_t const product{static_cast<uint32_t>(Traits::Top()) * duty};
    return static_cast<uint8_t>((product + internal::kDutyRounding) / kDutyMax);
  }

  void ApplyPeriod(uint8_t top, ClockSelect clock_select) {
    clock_select_ = clock_select;
    Traits::Top() = top;
    ApplyDuties(ChannelIndex<0U>{});
    if (state_ == State::kRunning) {
      SelectClock(clock_select_);
    }
  }

  // The duty of each channel at the top, from the one at Index on.
  template <uint8_t Index>
  void ApplyDuties(ChannelIndex<Index> /*index*/) {
    constexpr Channel kOutput{static_cast<Channel>(
        static_cast<uint8_t>(Traits::kFirstChannel) + Index)};
    PwmChannelOf<kOutput>::Compare() = CompareValue(duties_[Index]);
    ApplyDuties(ChannelIndex<static_cast<uint8_t>(Index + 1U)>{});
  }

  // Past the last channel: chosen over the template, as it is no template.
  void ApplyDuties(ChannelIndex<Traits::kChannelCount> /*end*/) {}

  // The flag of an overflow stays set until the interrupt runs, even while it
  // is disabled: the core's init() runs Timer2 for analogWrite() before
  // setup(), and an earlier run leaves it as well. Enabled with it set, the
  // interrupt would call the callback at once rather than after a period. A
  // flag is cleared by writing 1 to it, and only it: no read first.
  static void EnableOverflowInterrupt() {
    Traits::Tifr() = static_cast<uint8_t>(1U << Traits::kOverflowFlagBit);
    SetBit(Traits::Timsk(), Traits::kOverflowInterruptBit);
  }

  static void SelectClock(ClockSelect clock_select) {
    Traits::Tccrb() = static_cast<uint8_t>(
        (static_cast<unsigned>(Traits::Tccrb()) &
         ~static_cast<unsigned>(internal::kClockSelectMask)) |
        static_cast<unsigned>(clock_select));
  }

  template <typename Register>
  static void SetBit(Register& reg, uint8_t bit) {
    reg = static_cast<uint8_t>(static_cast<unsigned>(reg) | (1U << bit));
  }

  template <typename Register>
  static void ClearBit(Register& reg, uint8_t bit) {
    reg = static_cast<uint8_t>(static_cast<unsigned>(reg) & ~(1U << bit));
  }

  // volatile, as the overflow interrupt reads it: the store must not move
  // past the enabling of the interrupt.
  Callback volatile callback_{nullptr};
  State state_{State::kNotBegun};
  ClockSelect clock_select_{ClockSelect::kNoClock};
  // The duty SetPwmDuty() was given to each channel, which a new period
  // applies to its top.
  internal::array<uint16_t, Traits::kChannelCount> duties_{};
};

/*! \cond */
// The definitions C++11 needs for a static constexpr member that is used by
// reference.
template <typename Traits>
constexpr typename Timer<Traits>::TimeType Timer<Traits>::kPeriodMax;
template <typename Traits>
constexpr typename Timer<Traits>::TimeType Timer<Traits>::kDefaultPeriod;
/*! \endcond */

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER_H_
