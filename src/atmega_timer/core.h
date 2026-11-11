// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief internal::Core, what every mode of a timer shares: its states, its
 *        callback, its prescaler, and a period that waits for the bottom.
 *
 * A mode, as PhaseCorrectPwm, derives from internal::Core and adds what it
 * does with the counter: how a period sets the top, what the channels give,
 * and how the time is read. What differs from one Timer/Counter to another is
 * in its traits: timer1.h and timer2.h hand in those of Timer/Counter1 and
 * Timer/Counter2 of the ATmega328P.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_CORE_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_CORE_H_

#include <stdint.h>
#include <util/atomic.h>

#include "atmega_timer/attributes.h"
#include "atmega_timer/status.h"

namespace atmega_timer {

/*!
 * \brief A channel of a timer: its compare output, which drives a pin of its
 *        own.
 */
enum class Channel : uint8_t {
  kA, /*!< OCxA. */
  kB  /*!< OCxB, after OCxA: the traits count the channels in this order. */
};

/*! \brief The function the interrupt of a timer calls. */
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

// The number of values of a counter: 256 of 8 bits, 65536 of 16.
template <typename Counter>
constexpr uint32_t CounterValues() {
  return static_cast<uint32_t>(static_cast<Counter>(-1)) + 1U;
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

/*!
 * \brief What every mode of a timer shares: Begin() first, then Start(); a
 *        period, a callback, and the states between them.
 *
 * A function that can refuse returns a Status, and leaves the timer as it was
 * when it refuses. A sketch uses a mode, as the timer1 or timer2 that timer1.h
 * and timer2.h give, never this class itself.
 *
 * \tparam Derived the mode, which derives from this class: `kPeriodMax`, its
 *                 longest period; `TopOf()`, the top and the prescaler of a
 *                 period; and `ApplyChannels()`, which writes the compare value
 *                 of each channel for top().
 * \tparam Traits the Timer/Counter, as Timer2Traits gives it: `Counter`, the
 *                type of its counter, 8 or 16 bits; `TimeType`; `ClockSelect`,
 *                its prescalers, `kNoClock` at 0 and `kDivide1` at 1, the
 *                others in the values after it up to `kClockSelectLargest`,
 *                with `kShiftLargest` and `ShiftOf()`; for each channel
 *                `CompareOutput<Which>`, with `kPin`, `kCompareOutputBit` and
 *                `Compare()`; and `Tccra()`, `Tccrb()`, `Top()`, `Timsk()`,
 *                `Tifr()` and `Tcnt()`, which return its registers.
 * \tparam Mode the Timer/Counter in the mode, as
 *              Timer2Traits::PhaseCorrectPwmMode gives it: `kModeTccra` and
 *              `kModeTccrb`, the bits of the mode; `kTopIsBuffered`, whether
 *              the hardware takes a new top at a turn of the counter, or at
 *              once as ICR1; `kInterruptBit` and `kFlagBit`, the interrupt of
 *              the mode; and `kFirstChannel` and `kChannelCount`, the channels
 *              the mode drives, one after the other in Channel.
 */
template <typename Derived, typename Traits, typename Mode>
class Core {
 public:
  /*! \brief A time in microseconds, long enough for the longest period. */
  using TimeType = typename Traits::TimeType;

  /*! \brief The period Begin() sets if none is given: 1 ms. */
  static constexpr TimeType kDefaultPeriod{1000U};

  /*!
   * \brief Sets the timer up in its mode, with a period and a callback; it does
   *        not count before Start().
   * \param period the period in microseconds, up to the mode's kPeriodMax
   * \param callback what the interrupt calls, or nullptr for none
   * \return Error::kBegunAlready, or Error::kPeriodOutOfRange as SetPeriod()
   */
  ATMEGATIMER_NODISCARD Status Begin(TimeType period = kDefaultPeriod,
                                     Callback callback = nullptr) {
    ClockSelect clock_select{ClockSelect::kNoClock};
    Counter const top{Derived::TopOf(period, clock_select)};
    if (state_ != State::kNotBegun) {
      return Error::kBegunAlready;
    }
    if ((period > Derived::kPeriodMax) || (top == 0U)) {
      return Error::kPeriodOutOfRange;
    }
    // No clock and no output yet.
    Traits::Tccra() = Mode::kModeTccra;
    Traits::Tccrb() = Mode::kModeTccrb;
    state_ = State::kIdle;
    ApplyPeriod(top, clock_select);
    if (callback != nullptr) {
      Discard(AttachInterrupt(callback));  // not nullptr, so it attaches
    }
    return Error::kOk;
  }

  /*!
   * \brief Sets the period of the interrupt and of the channels; what each
   *        channel gives keeps its share of it.
   *
   * On Timer1, while it runs or is stopped, the period comes at the next
   * bottom, at most one period later; until then the counter and Read() keep
   * the old one. Its top, ICR1, is not buffered: written below the counter, it
   * would let the counter run on to 65535. The interrupt takes the new period,
   * with no callback as well.
   *
   * \param period the period in microseconds, up to the mode's kPeriodMax
   * \return Error::kNotBegun, or Error::kPeriodOutOfRange for a period too
   *         long, or too short for one step of the counter: 0, or 1 below 2 MHz
   */
  ATMEGATIMER_NODISCARD Status SetPeriod(TimeType period) {
    ClockSelect clock_select{ClockSelect::kNoClock};
    Counter const top{Derived::TopOf(period, clock_select)};
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    // A top of 0 would stop the counter at 0.
    if ((period > Derived::kPeriodMax) || (top == 0U)) {
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
      EnableInterrupt();
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
   * \brief Sets the function the interrupt calls, and enables the interrupt if
   *        the timer runs or is stopped, so that Resume() goes on with it.
   * \param callback the function
   * \return Error::kNoCallback for nullptr
   */
  ATMEGATIMER_NODISCARD Status AttachInterrupt(Callback callback) {
    if (callback == nullptr) {
      return Error::kNoCallback;
    }
    // The AVR writes the pointer a byte at a time: the interrupt must not call
    // through half of the new one.
    ClearBit(Traits::Timsk(), Mode::kInterruptBit);
    callback_ = callback;
    // Stopped, the counter stands and comes to no bottom before Resume().
    bool const has_counted{(state_ == State::kRunning) ||
                           (state_ == State::kStopped)};
    if (has_counted) {
      EnableInterrupt();
    }
    return Error::kOk;
  }

  /*!
   * \brief Disables the interrupt and forgets its callback, so that Start()
   *        does not enable it again.
   */
  void DetachInterrupt() {
    ClearBit(Traits::Timsk(), Mode::kInterruptBit);
    // After the interrupt is off, which calls through it.
    callback_ = nullptr;
    if (PeriodWaits()) {
      KeepInterruptForThePeriod();
    }
  }

  /*! \brief Where the timer is in its life. */
  State GetState() const { return state_; }

  /*! \brief The function the interrupt calls, or nullptr. */
  Callback GetCallback() const { return callback_; }

  /*!
   * \brief Takes a period that waits for the bottom, and calls the callback;
   *        for the interrupt of the mode, which the source of each timer
   *        defines, as timer2.cpp.
   */
  void OnInterrupt() {
    if (PeriodWaits()) {
      TakeWaitingPeriod();
      SelectClock(clock_select_);
    }
    Callback const callback{callback_};
    if (callback != nullptr) {
      callback();
    }
  }

  // A timer is its hardware: a copy would drive the same registers twice.
  Core(Core const&) = delete;
  Core(Core&&) = delete;
  Core& operator=(Core const&) = delete;
  Core& operator=(Core&&) = delete;

 protected:
  /*! \cond */
  ~Core() = default;

  using ClockSelect = typename Traits::ClockSelect;
  using Counter = typename Traits::Counter;

  template <Channel Which>
  using CompareOutputOf = typename Traits::template CompareOutput<Which>;

  // A channel's place among those of the mode.
  static constexpr uint8_t IndexOf(Channel which) {
    return static_cast<uint8_t>(static_cast<uint8_t>(which) -
                                static_cast<uint8_t>(Mode::kFirstChannel));
  }

  // One before the first wraps around to 255, beyond the last as well.
  static constexpr bool HasChannel(Channel which) {
    return IndexOf(which) < Mode::kChannelCount;
  }

  // A channel by its place, to go through all of them when compiling: each
  // has a register of its own, which no index reaches at run time.
  template <uint8_t Index>
  struct ChannelIndex {};

  // The top the channels are shares of: the one that waits, if one does.
  Counter top() const { return top_; }

  // The prescaler the counter runs at now.
  ClockSelect clock_select() const { return clock_select_; }

  // A register of a byte is read and written in one access.
  static constexpr bool kOneAccess{sizeof(Counter) == 1U};

  // A register of the counter's width, read or written so that the interrupt
  // does not come between its two bytes: the AVR passes the high byte of a
  // 16-bit register through one TEMP register per timer, which the interrupt
  // uses as well when it writes the top.
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

  template <typename Register>
  static void SetBit(Register& reg, uint8_t bit) {
    reg = static_cast<uint8_t>(static_cast<unsigned>(reg) | (1U << bit));
  }

  template <typename Register>
  static void ClearBit(Register& reg, uint8_t bit) {
    reg = static_cast<uint8_t>(static_cast<unsigned>(reg) & ~(1U << bit));
  }
  /*! \endcond */

 private:
  // Made by its mode only, which derives from it.
  Core() = default;
  friend Derived;

  Derived& derived() { return static_cast<Derived&>(*this); }

  // A top that the hardware does not buffer waits for the bottom while the
  // counter may stand above it: while it runs, and while it is stopped, as
  // Resume() goes on from there.
  bool WaitsForTheBottom() const {
    bool const may_be_above{(state_ == State::kRunning) ||
                            (state_ == State::kStopped)};
    return !Mode::kTopIsBuffered && may_be_above;
  }

  // Whether a period waits for the bottom; never where the hardware buffers
  // the top, so that the compiler leaves out what takes one there.
  bool PeriodWaits() const { return !Mode::kTopIsBuffered && period_waits_; }

  void ApplyPeriod(Counter top, ClockSelect clock_select) {
    clock_select_ = clock_select;
    top_ = top;
    Traits::Top() = top;
    derived().ApplyChannels();
    if (state_ == State::kRunning) {
      SelectClock(clock_select_);
    }
  }

  // The compare values go at once, as the hardware buffers them up to the
  // bottom; the top and the prescaler wait for the interrupt there. It is off
  // meanwhile, as it reads what changes.
  void DeferPeriod(Counter top, ClockSelect clock_select) {
    ClearBit(Traits::Timsk(), Mode::kInterruptBit);
    top_ = top;
    waiting_clock_select_ = clock_select;
    period_waits_ = true;
    derived().ApplyChannels();
    EnableInterrupt();
  }

  // At the bottom, or with the interrupt off: the period that waits. The
  // interrupt stays on for a callback alone.
  void TakeWaitingPeriod() {
    Traits::Top() = top_;
    clock_select_ = waiting_clock_select_;
    period_waits_ = false;
    if (callback_ == nullptr) {
      ClearBit(Traits::Timsk(), Mode::kInterruptBit);
    }
  }

  // On again after DetachInterrupt() turned it off for the callback, a few
  // clocks ago: a flag set since is a bottom, not one that an earlier run left.
  static void KeepInterruptForThePeriod() {
    SetBit(Traits::Timsk(), Mode::kInterruptBit);
  }

  // The flag of the interrupt stays set until the interrupt runs, even while
  // it is disabled: the core's init() runs Timer2 for analogWrite() before
  // setup(), and an earlier run leaves it as well. Enabled with it set, the
  // interrupt would call the callback at once rather than after a period. A
  // flag is cleared by writing 1 to it, and only it: no read first.
  static void EnableInterrupt() {
    Traits::Tifr() = static_cast<uint8_t>(1U << Mode::kFlagBit);
    SetBit(Traits::Timsk(), Mode::kInterruptBit);
  }

  static void SelectClock(ClockSelect clock_select) {
    Traits::Tccrb() =
        static_cast<uint8_t>((static_cast<unsigned>(Traits::Tccrb()) &
                              ~static_cast<unsigned>(kClockSelectMask)) |
                             static_cast<unsigned>(clock_select));
  }

  // volatile, as the interrupt reads it: the store must not move past the
  // enabling of the interrupt.
  Callback volatile callback_{nullptr};
  State state_{State::kNotBegun};
  ClockSelect clock_select_{ClockSelect::kNoClock};
  Counter top_{0U};
  // The period that waits for the bottom, which the interrupt takes: top_ and
  // waiting_clock_select_. Written with the interrupt off.
  ClockSelect waiting_clock_select_{ClockSelect::kNoClock};
  bool volatile period_waits_{false};
};

/*! \cond */
// The definition C++11 needs for a static constexpr member that is used by
// reference.
template <typename Derived, typename Traits, typename Mode>
constexpr typename Core<Derived, Traits, Mode>::TimeType
    Core<Derived, Traits, Mode>::kDefaultPeriod;
/*! \endcond */

}  // namespace internal
}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_CORE_H_
