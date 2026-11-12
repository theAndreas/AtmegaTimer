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
 * CtcMode is mode 2, with OCR2A as its top, whose compare match A interrupt
 * comes at the top, and OC2A and OC2B as its toggles. Each register is returned
 * as the vendor's header declares it, so that the compiler writes it where the
 * datasheet says.
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

  /*! \brief Timer/Counter2 in CTC, mode 2, for Ctc. */
  struct CtcMode {
    /*! \brief TCCR2A in mode 2: WGM21. */
    static constexpr uint8_t kModeTccra{1U << WGM21};
    /*! \brief TCCR2B in mode 2: no bit of the mode, and no clock. */
    static constexpr uint8_t kModeTccrb{0U};
    /*! \brief Whether the hardware buffers the top: OCR2A is not in CTC:
     * written below the counter, it lets the counter run on to 255. */
    static constexpr bool kTopIsBuffered{false};
    /*! \brief The bit of TIMSK2 that enables the compare match A interrupt. */
    static constexpr uint8_t kInterruptBit{OCIE2A};
    /*! \brief The bit of TIFR2 that flags the compare match A, at the top. */
    static constexpr uint8_t kFlagBit{OCF2A};
    /*! \brief The first channel that toggles: A, at the top. */
    static constexpr Channel kFirstChannel{Channel::kA};
    /*! \brief The number of channels that toggle: A and B. */
    static constexpr uint8_t kChannelCount{2U};
    /*! \brief The first channel whose compare value is its own: B, as OCR2A is
     * the top. */
    static constexpr Channel kFirstOwnCompare{Channel::kB};
  };

  /*!
   * \brief The compare output of a channel: its pin, its bits and its register.
   * \tparam Which the channel, Channel::kA or Channel::kB
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

/*!
 * \brief OC2A, channel A of Timer2, which toggles at the top in CTC: its
 *        compare value is the top, OCR2A, so it gives no PWM.
 */
template <>
struct Timer2Traits::CompareOutput<Channel::kA> {
  /*! \brief The Arduino pin of OC2A, PB3. */
  static constexpr uint8_t kPin{11};
  /*! \brief The bit of TCCR2A that toggles OC2A at each match: COM2A0. */
  static constexpr uint8_t kToggleBit{COM2A0};
};

/*! \brief OC2B, channel B of Timer2, which gives the PWM. */
template <>
struct Timer2Traits::CompareOutput<Channel::kB> {
  /*! \brief The Arduino pin of OC2B, PD3. */
  static constexpr uint8_t kPin{3};
  /*! \brief The bit of TCCR2A that connects OC2B: COM2B1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM2B1};
  /*! \brief The bit of TCCR2A that toggles OC2B at each match: COM2B0. */
  static constexpr uint8_t kToggleBit{COM2B0};
  /*! \brief OCR2B, its compare value. */
  static auto Compare() -> decltype((OCR2B)) { return OCR2B; }
};

namespace internal {

/*! \cond */
// Defined by timer2.cpp and by timer2_ctc.cpp alike: a sketch that uses timer2
// and timer2_ctc fails to link, with this name in the linker's message,
// rather than have two objects drive one timer.
extern uint8_t const kOneObjectOnTimer2;
/*! \endcond */

}  // namespace internal

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER2_TRAITS_H_
