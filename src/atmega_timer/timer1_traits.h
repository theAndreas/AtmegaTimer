// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer1Traits, Timer/Counter1 of the ATmega328P and the ATmega2560 for
 *        the modes of a timer: its registers, its prescalers and its channels.
 *
 * Apart from timer1.h, so that another object of Timer/Counter1 takes them
 * without timer1 and its interrupt.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_TRAITS_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_TRAITS_H_

#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/core.h"
#include "atmega_timer/timer16_traits.h"

namespace atmega_timer {

namespace internal {

/*! \cond */
// The Arduino pins of OC1A and OC1B, which lie elsewhere on the Mega than on
// the Uno, as the core's pins_arduino.h of each has them.
#ifdef __AVR_ATmega2560__
constexpr uint8_t kOc1aPin{11};  // PB5
constexpr uint8_t kOc1bPin{12};  // PB6
#else
constexpr uint8_t kOc1aPin{9};   // PB1
constexpr uint8_t kOc1bPin{10};  // PB2
#endif
/*! \endcond */

}  // namespace internal

/*!
 * \brief What Timer/Counter1 has of its own, for Timer16Traits: its registers,
 *        its bits and its channels.
 *
 * Channels A and B. OC1C of the ATmega2560 is left out: its pin, 13 of the
 * Mega, joins it with OC0A of Timer0. Each register is returned as the
 * vendor's header declares it, so that the compiler writes it where the
 * datasheet says.
 */
struct Timer1Registers {
  /*! \brief The bit WGM12 of TCCR1B. */
  static constexpr uint8_t kWgm2{WGM12};
  /*! \brief The bit WGM13 of TCCR1B. */
  static constexpr uint8_t kWgm3{WGM13};
  /*! \brief The bit of TIMSK1 that enables the overflow interrupt: TOIE1. */
  static constexpr uint8_t kOverflowInterruptBit{TOIE1};
  /*! \brief The bit of TIFR1 that flags an overflow: TOV1. */
  static constexpr uint8_t kOverflowFlagBit{TOV1};
  /*! \brief The bit of TIMSK1 that enables the input capture interrupt:
   * ICIE1. */
  static constexpr uint8_t kCaptureInterruptBit{ICIE1};
  /*! \brief The bit of TIFR1 that flags the input capture: ICF1. */
  static constexpr uint8_t kCaptureFlagBit{ICF1};
  /*! \brief The number of channels: A and B. */
  static constexpr uint8_t kChannelCount{2u};

  /*!
   * \brief The compare output of a channel: its pin, its bits and its register.
   * \tparam Which the channel, Channel::kA or Channel::kB
   */
  template <Channel Which>
  struct CompareOutput;

  /*! \brief TCCR1A. */
  static auto Tccra() -> decltype((TCCR1A)) { return TCCR1A; }
  /*! \brief TCCR1B. */
  static auto Tccrb() -> decltype((TCCR1B)) { return TCCR1B; }
  /*! \brief ICR1. */
  static auto Icr() -> decltype((ICR1)) { return ICR1; }
  /*! \brief TIMSK1. */
  static auto Timsk() -> decltype((TIMSK1)) { return TIMSK1; }
  /*! \brief TIFR1. */
  static auto Tifr() -> decltype((TIFR1)) { return TIFR1; }
  /*! \brief TCNT1. */
  static auto Tcnt() -> decltype((TCNT1)) { return TCNT1; }
};

/*! \brief OC1A, channel A of Timer1. */
template <>
struct Timer1Registers::CompareOutput<Channel::kA> {
  /*! \brief The Arduino pin of OC1A: 9 on the Uno, 11 on the Mega. */
  static constexpr uint8_t kPin{internal::kOc1aPin};
  /*! \brief The bit of TCCR1A that connects OC1A: COM1A1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM1A1};
  /*! \brief The bit of TCCR1A that toggles OC1A at each match: COM1A0. */
  static constexpr uint8_t kToggleBit{COM1A0};
  /*! \brief OCR1A, its compare value. */
  static auto Compare() -> decltype((OCR1A)) { return OCR1A; }
};

/*! \brief OC1B, channel B of Timer1. */
template <>
struct Timer1Registers::CompareOutput<Channel::kB> {
  /*! \brief The Arduino pin of OC1B: 10 on the Uno, 12 on the Mega. */
  static constexpr uint8_t kPin{internal::kOc1bPin};
  /*! \brief The bit of TCCR1A that connects OC1B: COM1B1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM1B1};
  /*! \brief The bit of TCCR1A that toggles OC1B at each match: COM1B0. */
  static constexpr uint8_t kToggleBit{COM1B0};
  /*! \brief OCR1B, its compare value. */
  static auto Compare() -> decltype((OCR1B)) { return OCR1B; }
};

/*!
 * \brief Timer/Counter1 of the ATmega328P and the ATmega2560, for the modes of
 *        a timer: its registers, its prescalers and its channels.
 */
using Timer1Traits = Timer16Traits<Timer1Registers>;

namespace internal {

/*! \cond */
// Defined by timer1.cpp and by timer1_ctc.cpp alike: a sketch that uses timer1
// and timer1_ctc fails to link, with this name in the linker's message,
// rather than have two objects drive one timer.
extern uint8_t const kOneObjectOnTimer1;
/*! \endcond */

}  // namespace internal

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER1_TRAITS_H_
