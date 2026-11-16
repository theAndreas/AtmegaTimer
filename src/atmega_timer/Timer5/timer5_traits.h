// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer5Traits, Timer/Counter5 of the ATmega2560 for the modes of a
 *        timer: its registers, its prescalers and its channels.
 *
 * Only on the ATmega2560: on another board, this header declares nothing.
 * Apart from timer5.h, so that another object of Timer/Counter5 takes them
 * without timer5 and its interrupt.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_TIMER5_TRAITS_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_TIMER5_TRAITS_H_

#ifdef __AVR_ATmega2560__

#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/Modes/core.h"
#include "atmega_timer/Timer16/timer16_traits.h"

namespace atmega_timer {

/*!
 * \brief What Timer/Counter5 has of its own, for Timer16Traits: its registers,
 *        its bits and its channels.
 *
 * Channels A, B and C. Each register is returned as the vendor's header
 * declares it, so that the compiler writes it where the datasheet says.
 */
struct Timer5Registers {
  /*! \brief The bit WGM52 of TCCR5B. */
  static constexpr uint8_t kWgm2{WGM52};
  /*! \brief The bit WGM53 of TCCR5B. */
  static constexpr uint8_t kWgm3{WGM53};
  /*! \brief The bit of TIMSK5 that enables the overflow interrupt: TOIE5. */
  static constexpr uint8_t kOverflowInterruptBit{TOIE5};
  /*! \brief The bit of TIFR5 that flags an overflow: TOV5. */
  static constexpr uint8_t kOverflowFlagBit{TOV5};
  /*! \brief The bit of TIMSK5 that enables the input capture interrupt:
   * ICIE5. */
  static constexpr uint8_t kCaptureInterruptBit{ICIE5};
  /*! \brief The bit of TIFR5 that flags the input capture: ICF5. */
  static constexpr uint8_t kCaptureFlagBit{ICF5};
  /*! \brief The number of channels: A, B and C. */
  static constexpr uint8_t kChannelCount{3u};

  /*!
   * \brief The compare output of a channel: its pin, its bits and its register.
   * \tparam Which the channel, Channel::kA, Channel::kB or Channel::kC
   */
  template <Channel Which>
  struct CompareOutput;

  /*! \brief TCCR5A. */
  static auto Tccra() -> decltype((TCCR5A)) { return TCCR5A; }
  /*! \brief TCCR5B. */
  static auto Tccrb() -> decltype((TCCR5B)) { return TCCR5B; }
  /*! \brief ICR5. */
  static auto Icr() -> decltype((ICR5)) { return ICR5; }
  /*! \brief TIMSK5. */
  static auto Timsk() -> decltype((TIMSK5)) { return TIMSK5; }
  /*! \brief TIFR5. */
  static auto Tifr() -> decltype((TIFR5)) { return TIFR5; }
  /*! \brief TCNT5. */
  static auto Tcnt() -> decltype((TCNT5)) { return TCNT5; }
};

/*! \brief OC5A, channel A of Timer5. */
template <>
struct Timer5Registers::CompareOutput<Channel::kA> {
  /*! \brief The Arduino pin of OC5A, PL3: 46 on the Mega. */
  static constexpr uint8_t kPin{46};
  /*! \brief The bit of TCCR5A that connects OC5A: COM5A1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM5A1};
  /*! \brief The bit of TCCR5A that toggles OC5A at each match: COM5A0. */
  static constexpr uint8_t kToggleBit{COM5A0};
  /*! \brief OCR5A, its compare value. */
  static auto Compare() -> decltype((OCR5A)) { return OCR5A; }
};

/*! \brief OC5B, channel B of Timer5. */
template <>
struct Timer5Registers::CompareOutput<Channel::kB> {
  /*! \brief The Arduino pin of OC5B, PL4: 45 on the Mega. */
  static constexpr uint8_t kPin{45};
  /*! \brief The bit of TCCR5A that connects OC5B: COM5B1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM5B1};
  /*! \brief The bit of TCCR5A that toggles OC5B at each match: COM5B0. */
  static constexpr uint8_t kToggleBit{COM5B0};
  /*! \brief OCR5B, its compare value. */
  static auto Compare() -> decltype((OCR5B)) { return OCR5B; }
};

/*! \brief OC5C, channel C of Timer5. */
template <>
struct Timer5Registers::CompareOutput<Channel::kC> {
  /*! \brief The Arduino pin of OC5C, PL5: 44 on the Mega. */
  static constexpr uint8_t kPin{44};
  /*! \brief The bit of TCCR5A that connects OC5C: COM5C1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM5C1};
  /*! \brief The bit of TCCR5A that toggles OC5C at each match: COM5C0. */
  static constexpr uint8_t kToggleBit{COM5C0};
  /*! \brief OCR5C, its compare value. */
  static auto Compare() -> decltype((OCR5C)) { return OCR5C; }
};

/*!
 * \brief Timer/Counter5 of the ATmega2560, for the modes of a timer: its
 *        registers, its prescalers and its channels.
 */
using Timer5Traits = Timer16Traits<Timer5Registers>;

namespace internal {

/*! \cond */
// Defined by timer5.cpp and by timer5_ctc.cpp alike: a sketch that uses
// timer5 and timer5_ctc fails to link, with this name in the linker's
// message, rather than have two objects drive one timer.
extern uint8_t const kOneObjectOnTimer5;
/*! \endcond */

}  // namespace internal

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER5_TIMER5_TRAITS_H_
