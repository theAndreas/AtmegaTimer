// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer1, Timer/Counter1 of the ATmega328P as a Timer.
 *
 * It breaks `Servo`, which runs on Timer1 on the ATmega328P, and analogWrite()
 * on pins 9 and 10. Both channels give a PWM of their own: ICR1 is the top.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_H_

#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/timer.h"

namespace atmega_timer {

/*!
 * \brief Timer/Counter1 of the ATmega328P, for Timer: in phase and frequency
 *        correct PWM, mode 8, with ICR1 as its top and OC1A and OC1B as its
 *        PWMs.
 *
 * Mode 8 takes the compare values at the bottom, so the PWM stays symmetric
 * when the period changes. Each register is returned as the vendor's header
 * declares it, so that the compiler writes it where the datasheet says.
 */
struct Timer1Traits {
  /*! \brief The type of the counter and its registers: 16 bits. */
  using Counter = uint16_t;

  /*!
   * \brief A time in microseconds: 32 bits, as the longest period is about 8.4
   *        s at 16 MHz.
   */
  using TimeType = uint32_t;

  /*! \brief The values of the clock select bits CS12..CS10. */
  enum class ClockSelect : uint8_t {
    kNoClock = 0, /*!< The counter stands. */
    kDivide1,     /*!< The CPU's clock. */
    kDivide8,     /*!< The CPU's clock / 8. */
    kDivide64,    /*!< The CPU's clock / 64. */
    kDivide256,   /*!< The CPU's clock / 256. */
    kDivide1024   /*!< The CPU's clock / 1024. */
  };

  /*! \brief The largest prescaler. */
  static constexpr ClockSelect kClockSelectLargest{ClockSelect::kDivide1024};
  /*! \brief The largest prescaler as a shift: 1024 is 2^10. */
  static constexpr uint8_t kShiftLargest{10U};

  /*!
   * \brief Returns the prescaler of `clock_select` as a shift, 0 for none.
   * \param[in] clock_select the prescaler
   * \return the power of 2 it divides by
   */
  static uint8_t ShiftOf(ClockSelect clock_select);

  /*! \brief TCCR1A in mode 8: no bit of the mode. */
  static constexpr uint8_t kModeTccra{0U};
  /*! \brief TCCR1B in mode 8: WGM13, and no clock. */
  static constexpr uint8_t kModeTccrb{1U << WGM13};
  /*! \brief The bit of TIMSK1 that enables the overflow interrupt. */
  static constexpr uint8_t kOverflowInterruptBit{TOIE1};
  /*! \brief The bit of TIFR1 that flags an overflow, at the bottom. */
  static constexpr uint8_t kOverflowFlagBit{TOV1};

  /*! \brief The first channel that gives a PWM: A, as ICR1 is the top. */
  static constexpr Channel kFirstChannel{Channel::kA};
  /*! \brief The number of channels that give a PWM: A and B. */
  static constexpr uint8_t kChannelCount{2U};

  /*!
   * \brief A channel that gives a PWM.
   * \tparam Output the channel, Channel::kA or Channel::kB
   */
  template <Channel Output>
  struct PwmChannel;

  /*! \brief TCCR1A. */
  static auto Tccra() -> decltype((TCCR1A)) { return TCCR1A; }
  /*! \brief TCCR1B. */
  static auto Tccrb() -> decltype((TCCR1B)) { return TCCR1B; }
  /*! \brief ICR1, the top; not buffered, unlike the compare values. */
  static auto Top() -> decltype((ICR1)) { return ICR1; }
  /*! \brief TIMSK1. */
  static auto Timsk() -> decltype((TIMSK1)) { return TIMSK1; }
  /*! \brief TIFR1, whose flags are cleared by writing 1. */
  static auto Tifr() -> decltype((TIFR1)) { return TIFR1; }
  /*! \brief TCNT1, the counter. */
  static auto Tcnt() -> decltype((TCNT1)) { return TCNT1; }
};

/*! \brief OC1A, channel A of Timer1. */
template <>
struct Timer1Traits::PwmChannel<Channel::kA> {
  /*! \brief The Arduino pin of OC1A, PB1. */
  static constexpr uint8_t kPin{9};
  /*! \brief The bit of TCCR1A that connects OC1A: COM1A1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM1A1};
  /*! \brief OCR1A, the compare value of its PWM. */
  static auto Compare() -> decltype((OCR1A)) { return OCR1A; }
};

/*! \brief OC1B, channel B of Timer1. */
template <>
struct Timer1Traits::PwmChannel<Channel::kB> {
  /*! \brief The Arduino pin of OC1B, PB2. */
  static constexpr uint8_t kPin{10};
  /*! \brief The bit of TCCR1A that connects OC1B: COM1B1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM1B1};
  /*! \brief OCR1B, the compare value of its PWM. */
  static auto Compare() -> decltype((OCR1B)) { return OCR1B; }
};

/*! \brief The type of timer1, for its constants: `Timer1::kPeriodMax`. */
using Timer1 = Timer<Timer1Traits>;

/*!
 * \brief Timer/Counter1; its overflow interrupt calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer1 timer1;

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_H_
