// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer4Traits, Timer/Counter4 of the ATmega2560 for the modes of a
 *        timer: its registers, its prescalers and its channels.
 *
 * Only on the ATmega2560: on another board, this header declares nothing.
 * Apart from timer4.h, so that another object of Timer/Counter4 takes them
 * without timer4 and its interrupt.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_TIMER4_TRAITS_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_TIMER4_TRAITS_H_

#ifdef __AVR_ATmega2560__

#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/Modes/core.h"
#include "atmega_timer/Timer16/timer16_traits.h"

namespace atmega_timer {

/*!
 * \brief What Timer/Counter4 has of its own, for Timer16Traits: its registers,
 *        its bits and its channels.
 *
 * Channels A, B and C. Each register is returned as the vendor's header
 * declares it, so that the compiler writes it where the datasheet says.
 */
struct Timer4Registers {
  /*! \brief The bit WGM42 of TCCR4B. */
  static constexpr uint8_t kWgm2{WGM42};
  /*! \brief The bit WGM43 of TCCR4B. */
  static constexpr uint8_t kWgm3{WGM43};
  /*! \brief The bit of TIMSK4 that enables the overflow interrupt: TOIE4. */
  static constexpr uint8_t kOverflowInterruptBit{TOIE4};
  /*! \brief The bit of TIFR4 that flags an overflow: TOV4. */
  static constexpr uint8_t kOverflowFlagBit{TOV4};
  /*! \brief The bit of TIMSK4 that enables the input capture interrupt:
   * ICIE4. */
  static constexpr uint8_t kCaptureInterruptBit{ICIE4};
  /*! \brief The bit of TIFR4 that flags the input capture: ICF4. */
  static constexpr uint8_t kCaptureFlagBit{ICF4};
  /*! \brief The number of channels: A, B and C. */
  static constexpr uint8_t kChannelCount{3u};

  /*!
   * \brief The compare output of a channel: its pin, its bits and its register.
   * \tparam Which the channel, Channel::kA, Channel::kB or Channel::kC
   */
  template <Channel Which>
  struct CompareOutput;

  /*! \brief TCCR4A. */
  static auto Tccra() -> decltype((TCCR4A)) { return TCCR4A; }
  /*! \brief TCCR4B. */
  static auto Tccrb() -> decltype((TCCR4B)) { return TCCR4B; }
  /*! \brief ICR4. */
  static auto Icr() -> decltype((ICR4)) { return ICR4; }
  /*! \brief TIMSK4. */
  static auto Timsk() -> decltype((TIMSK4)) { return TIMSK4; }
  /*! \brief TIFR4. */
  static auto Tifr() -> decltype((TIFR4)) { return TIFR4; }
  /*! \brief TCNT4. */
  static auto Tcnt() -> decltype((TCNT4)) { return TCNT4; }
};

/*! \brief OC4A, channel A of Timer4. */
template <>
struct Timer4Registers::CompareOutput<Channel::kA> {
  /*! \brief The Arduino pin of OC4A, PH3: 6 on the Mega. */
  static constexpr uint8_t kPin{6};
  /*! \brief The bit of TCCR4A that connects OC4A: COM4A1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM4A1};
  /*! \brief The bit of TCCR4A that toggles OC4A at each match: COM4A0. */
  static constexpr uint8_t kToggleBit{COM4A0};
  /*! \brief OCR4A, its compare value. */
  static auto Compare() -> decltype((OCR4A)) { return OCR4A; }
};

/*! \brief OC4B, channel B of Timer4. */
template <>
struct Timer4Registers::CompareOutput<Channel::kB> {
  /*! \brief The Arduino pin of OC4B, PH4: 7 on the Mega. */
  static constexpr uint8_t kPin{7};
  /*! \brief The bit of TCCR4A that connects OC4B: COM4B1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM4B1};
  /*! \brief The bit of TCCR4A that toggles OC4B at each match: COM4B0. */
  static constexpr uint8_t kToggleBit{COM4B0};
  /*! \brief OCR4B, its compare value. */
  static auto Compare() -> decltype((OCR4B)) { return OCR4B; }
};

/*! \brief OC4C, channel C of Timer4. */
template <>
struct Timer4Registers::CompareOutput<Channel::kC> {
  /*! \brief The Arduino pin of OC4C, PH5: 8 on the Mega. */
  static constexpr uint8_t kPin{8};
  /*! \brief The bit of TCCR4A that connects OC4C: COM4C1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM4C1};
  /*! \brief The bit of TCCR4A that toggles OC4C at each match: COM4C0. */
  static constexpr uint8_t kToggleBit{COM4C0};
  /*! \brief OCR4C, its compare value. */
  static auto Compare() -> decltype((OCR4C)) { return OCR4C; }
};

/*!
 * \brief Timer/Counter4 of the ATmega2560, for the modes of a timer: its
 *        registers, its prescalers and its channels.
 */
using Timer4Traits = Timer16Traits<Timer4Registers>;

namespace internal {

/*! \cond */
// Defined by timer4.cpp and by timer4_ctc.cpp alike: a sketch that uses
// timer4 and timer4_ctc fails to link, with this name in the linker's
// message, rather than have two objects drive one timer.
extern uint8_t const kOneObjectOnTimer4;
/*! \endcond */

}  // namespace internal

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER4_TIMER4_TRAITS_H_
