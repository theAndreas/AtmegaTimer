// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer, a periodic interrupt, a PWM and a time readout on a
 *        Timer/Counter of the AVR in phase correct PWM.
 *
 * The counter runs up to the top and down again: the period sets the top and
 * the prescaler; the overflow at the bottom calls the callback; and a channel
 * gives the PWM, at a duty that is a share of the top. What differs from one
 * Timer/Counter to another is in its traits, a parameter of the template:
 * timer1.h and timer2.h hand in those of Timer/Counter1 and Timer/Counter2 of
 * the ATmega328P.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER_H_

#include <Arduino.h>
#include <avr/io.h>
#include <stdint.h>
#include <util/atomic.h>

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

constexpr uint32_t kClocksPerMillisecond{F_CPU / 1000UL};
constexpr uint32_t kMicrosecondsPerMillisecond{1000U};
// The duty is a share of kDutyMax of the top, rounded.
constexpr uint32_t kDutyRounding{kDutyMax / 2U};

// The number of values of a counter: 256 of 8 bits, 65536 of 16.
template <typename Counter>
constexpr uint32_t CounterValues() {
  return static_cast<uint32_t>(static_cast<Counter>(-1)) + 1U;
}

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

// The microseconds of clocks, rounded down. Its whole milliseconds and the
// clocks left apart, so that no product leaves 32 bits: 2 * 65535 steps of
// 1024 clocks, times 1000, would.
constexpr uint32_t MicrosecondsOf(uint32_t clocks) {
  return ((clocks / kClocksPerMillisecond) * kMicrosecondsPerMillisecond) +
         (((clocks % kClocksPerMillisecond) * kMicrosecondsPerMillisecond) /
          kClocksPerMillisecond);
}

/*! \endcond */

}  // namespace internal

/*!
 * \brief A periodic interrupt, a PWM and a time readout on a Timer/Counter in
 *        phase correct PWM: mode 8 on Timer1, mode 5 on Timer2.
 *
 * Begin() first, then Start(). A function that can refuse returns a Status,
 * and leaves the timer as it was when it refuses. A sketch uses the one that
 * timer1.h or timer2.h gives, timer1 or timer2.
 *
 * \tparam Traits the Timer/Counter, as Timer2Traits gives it: `Counter`, the
 *                type of its counter, 8 or 16 bits; `TimeType`; `ClockSelect`,
 *                its prescalers, `kNoClock` at 0 and `kDivide1` at 1, the
 *                others in the values after it up to `kClockSelectLargest`,
 *                with `kShiftLargest` and `ShiftOf()`; `kModeTccra` and
 *                `kModeTccrb`, the bits of the mode; `kTopIsBuffered`, whether
 *                the hardware takes a new top at a turn of the counter, or at
 *                once as ICR1; `kOverflowInterruptBit` and `kOverflowFlagBit`;
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
    Counter const top{TopOf(period, clock_select)};
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
   * \brief Sets the period of the interrupt and of the PWM; the duty of each
   *        channel keeps its share of it.
   *
   * On Timer1, while it runs or is stopped, the period comes at the next
   * bottom, at most one period later; until then the counter and Read() keep
   * the old one. Its top, ICR1, is not buffered: written below the counter, it
   * would let the counter run on to 65535. The overflow interrupt takes the new
   * period, with no callback as well.
   *
   * \param period the period in microseconds, up to kPeriodMax
   * \return Error::kNotBegun, or Error::kPeriodOutOfRange for a period too
   *         long, or too short for one step of the counter: 0, or 1 below 2 MHz
   */
  ATMEGATIMER_NODISCARD Status SetPeriod(TimeType period) {
    ClockSelect clock_select{ClockSelect::kNoClock};
    Counter const top{TopOf(period, clock_select)};
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    // A top of 0 would stop the counter at 0.
    if ((period > kPeriodMax) || (top == 0U)) {
      return Error::kPeriodOutOfRange;
    }
    if (WaitsForTheBottom()) {
      DeferPeriod(top, clock_select);
    } else {
      ApplyPeriod(top, clock_select);
    }
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
    // At 0 the counter is at its bottom, which a period waits for.
    if (PeriodWaits()) {
      TakeWaitingPeriod();
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
    } else if (PeriodWaits()) {
      KeepOverflowInterruptForThePeriod();
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
    if (PeriodWaits()) {
      KeepOverflowInterruptForThePeriod();
    }
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
    Store(PwmChannelOf<Output>::Compare(), CompareValue(duty));
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
    // Up and down, the steps of a 16-bit counter take 17 bits.
    uint32_t steps{Load(Traits::Tcnt())};
    Counter next{Load(Traits::Tcnt())};
    while (next == steps) {
      next = Load(Traits::Tcnt());
    }
    // Counting down: up to the top, and back to where it is.
    if (next < steps) {
      uint32_t const top{Load(Traits::Top())};
      steps = (2U * top) - steps;
    }
    // The time is below kPeriodMax, which TimeType holds.
    uint32_t const clocks{steps << Traits::ShiftOf(clock_select_)};
    return static_cast<TimeType>(internal::MicrosecondsOf(clocks));
  }

  /*! \brief Where the timer is in its life. */
  State GetState() const { return state_; }

  /*! \brief The function the overflow interrupt calls, or nullptr. */
  Callback GetCallback() const { return callback_; }

  /*!
   * \brief Takes a period that waits for the bottom, and calls the callback;
   *        for the overflow interrupt, which the source of each timer defines,
   *        as timer2.cpp.
   */
  void OnOverflow() {
    if (PeriodWaits()) {
      TakeWaitingPeriod();
      SelectClock(clock_select_);
    }
    Callback const callback{callback_};
    if (callback != nullptr) {
      callback();
    }
  }

 private:
  using ClockSelect = typename Traits::ClockSelect;
  using Counter = typename Traits::Counter;

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

  // The compare value of a duty at top_, rounded: 0 is the bottom, which keeps
  // the output low, and kDutyMax the top, which keeps it high. In 32 bits:
  // 255 * 1023 does not fit 16, 65535 * 1023 does 32.
  Counter CompareValue(uint16_t duty) const {
    uint32_t const product{static_cast<uint32_t>(top_) * duty};
    return static_cast<Counter>((product + internal::kDutyRounding) / kDutyMax);
  }

  // A top that the hardware does not buffer waits for the bottom while the
  // counter may stand above it: while it runs, and while it is stopped, as
  // Resume() goes on from there.
  bool WaitsForTheBottom() const {
    bool const may_be_above{(state_ == State::kRunning) ||
                            (state_ == State::kStopped)};
    return !Traits::kTopIsBuffered && may_be_above;
  }

  // Whether a period waits for the bottom; never where the hardware buffers
  // the top, so that the compiler leaves out what takes one there.
  bool PeriodWaits() const { return !Traits::kTopIsBuffered && period_waits_; }

  void ApplyPeriod(Counter top, ClockSelect clock_select) {
    clock_select_ = clock_select;
    top_ = top;
    Traits::Top() = top;
    ApplyDuties(ChannelIndex<0U>{});
    if (state_ == State::kRunning) {
      SelectClock(clock_select_);
    }
  }

  // The compare values go at once, as the hardware buffers them up to the
  // bottom; the top and the prescaler wait for the overflow interrupt there.
  // It is off meanwhile, as it reads what changes.
  void DeferPeriod(Counter top, ClockSelect clock_select) {
    ClearBit(Traits::Timsk(), Traits::kOverflowInterruptBit);
    top_ = top;
    waiting_clock_select_ = clock_select;
    period_waits_ = true;
    ApplyDuties(ChannelIndex<0U>{});
    EnableOverflowInterrupt();
  }

  // At the bottom, or with the overflow interrupt off: the period that waits.
  // The interrupt stays on for a callback alone.
  void TakeWaitingPeriod() {
    Traits::Top() = top_;
    clock_select_ = waiting_clock_select_;
    period_waits_ = false;
    if (callback_ == nullptr) {
      ClearBit(Traits::Timsk(), Traits::kOverflowInterruptBit);
    }
  }

  // On again after AttachInterrupt() or DetachInterrupt() turned it off for
  // the callback, a few clocks ago: an overflow flagged since is a bottom, not
  // one that an earlier run left.
  static void KeepOverflowInterruptForThePeriod() {
    SetBit(Traits::Timsk(), Traits::kOverflowInterruptBit);
  }

  // A register of the counter's width, read or written so that the overflow
  // interrupt does not come between its two bytes: the AVR passes the high
  // byte of a 16-bit register through one TEMP register per timer, which the
  // interrupt uses as well when it writes the top.
  // A register of a byte is read and written in one access.
  static constexpr bool kOneAccess{sizeof(Counter) == 1U};

  template <typename Register>
  static Counter Load(Register& reg) {
    if (kOneAccess) {
      return reg;
    }
    Counter value{0U};
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = reg; }
    return value;
  }

  template <typename Register>
  static void Store(Register& reg, Counter value) {
    if (kOneAccess) {
      reg = value;
      return;
    }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { reg = value; }
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
  // The top the duties are shares of: the one that waits, if one does.
  Counter top_{0U};
  // The period that waits for the bottom, which the overflow interrupt takes:
  // top_ and waiting_clock_select_. Written with the interrupt off.
  ClockSelect waiting_clock_select_{ClockSelect::kNoClock};
  bool volatile period_waits_{false};
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
