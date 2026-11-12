// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer2Traits, Timer/Counter2 of the ATmega328P for the modes of a
 *        timer: its registers, its prescalers and its channels.
 *
 * Apart from timer2.h, so that another object of Timer/Counter2 takes them
 * without timer2 and its interrupt.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_TRAITS_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_TRAITS_H_

#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/core.h"

namespace atmega_timer {

/*!
 * \brief Timer/Counter2 of the ATmega328P, for the modes of a timer: its
 *        registers, its prescalers and its channels.
 *
 * PhaseCorrectPwmMode is mode 5, with OCR2A as its top and OC2B as its PWM.
 * Each register is returned as the vendor's header declares it, so that the
 * compiler writes it where the datasheet says.
 */
struct Timer2Traits {
  /*! \brief The type of the counter and its registers: 8 bits. */
  using Counter = uint8_t;

  /*!
   * \brief A time in microseconds: 32 bits below 8 MHz, where the longest
   *        period does not fit into 16.
   */
#if F_CPU < 8000000UL
  using TimeType = uint32_t;
#else
  using TimeType = uint16_t;
#endif

  /*! \brief The values of the clock select bits CS22..CS20. */
  enum class ClockSelect : uint8_t {
    kNoClock = 0, /*!< The counter stands. */
    kDivide1,     /*!< The CPU's clock. */
    kDivide8,     /*!< The CPU's clock / 8. */
    kDivide32,    /*!< The CPU's clock / 32. */
    kDivide64,    /*!< The CPU's clock / 64. */
    kDivide128,   /*!< The CPU's clock / 128. */
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

  /*! \brief Timer/Counter2 in phase correct PWM, mode 5, for PhaseCorrectPwm.
   */
  struct PhaseCorrectPwmMode {
    /*! \brief TCCR2A in mode 5: WGM20. */
    static constexpr uint8_t kModeTccra{1U << WGM20};
    /*! \brief TCCR2B in mode 5: WGM22, and no clock. */
    static constexpr uint8_t kModeTccrb{1U << WGM22};
    /*! \brief Whether the hardware buffers the top: OCR2A is, and taken at the
     * top in mode 5. */
    static constexpr bool kTopIsBuffered{true};
    /*! \brief The bit of TIMSK2 that enables the overflow interrupt. */
    static constexpr uint8_t kInterruptBit{TOIE2};
    /*! \brief The bit of TIFR2 that flags an overflow. */
    static constexpr uint8_t kFlagBit{TOV2};
    /*! \brief The first channel that gives a PWM: B, as OCR2A is the top. */
    static constexpr Channel kFirstChannel{Channel::kB};
    /*! \brief The number of channels that give a PWM: B alone. */
    static constexpr uint8_t kChannelCount{1U};
  };

  /*!
   * \brief The compare output of a channel: its pin, its bits and its register.
   * \tparam Which the channel, Channel::kB alone
   */
  template <Channel Which>
  struct CompareOutput;

  /*! \brief TCCR2A. */
  static auto Tccra() -> decltype((TCCR2A)) { return TCCR2A; }
  /*! \brief TCCR2B. */
  static auto Tccrb() -> decltype((TCCR2B)) { return TCCR2B; }
  /*! \brief OCR2A, the top. */
  static auto Top() -> decltype((OCR2A)) { return OCR2A; }
  /*! \brief TIMSK2. */
  static auto Timsk() -> decltype((TIMSK2)) { return TIMSK2; }
  /*! \brief TIFR2, whose flags are cleared by writing 1. */
  static auto Tifr() -> decltype((TIFR2)) { return TIFR2; }
  /*! \brief TCNT2, the counter. */
  static auto Tcnt() -> decltype((TCNT2)) { return TCNT2; }
};

/*! \brief OC2B, the channel of Timer2 that gives the PWM. */
template <>
struct Timer2Traits::CompareOutput<Channel::kB> {
  /*! \brief The Arduino pin of OC2B, PD3. */
  static constexpr uint8_t kPin{3};
  /*! \brief The bit of TCCR2A that connects OC2B: COM2B1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM2B1};
  /*! \brief OCR2B, the compare value of the PWM. */
  static auto Compare() -> decltype((OCR2B)) { return OCR2B; }
};

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_TRAITS_H_
