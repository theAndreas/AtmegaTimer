// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer3Traits, Timer/Counter3 of the ATmega2560 for the modes of a
 *        timer: its registers, its prescalers and its channels.
 *
 * Only on the ATmega2560: on another board, this header declares nothing.
 * Apart from timer3.h, so that another object of Timer/Counter3 takes them
 * without timer3 and its interrupt.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_TRAITS_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_TRAITS_H_

#ifdef __AVR_ATmega2560__

#include <avr/io.h>
#include <stdint.h>

#include "atmega_timer/core.h"
#include "atmega_timer/timer16_traits.h"

namespace atmega_timer {

/*!
 * \brief What Timer/Counter3 has of its own, for Timer16Traits: its registers,
 *        its bits and its channels.
 *
 * Channels A, B and C. Each register is returned as the vendor's header
 * declares it, so that the compiler writes it where the datasheet says.
 */
struct Timer3Registers {
  /*! \brief The bit WGM32 of TCCR3B. */
  static constexpr uint8_t kWgm2{WGM32};
  /*! \brief The bit WGM33 of TCCR3B. */
  static constexpr uint8_t kWgm3{WGM33};
  /*! \brief The bit of TIMSK3 that enables the overflow interrupt: TOIE3. */
  static constexpr uint8_t kOverflowInterruptBit{TOIE3};
  /*! \brief The bit of TIFR3 that flags an overflow: TOV3. */
  static constexpr uint8_t kOverflowFlagBit{TOV3};
  /*! \brief The bit of TIMSK3 that enables the input capture interrupt:
   * ICIE3. */
  static constexpr uint8_t kCaptureInterruptBit{ICIE3};
  /*! \brief The bit of TIFR3 that flags the input capture: ICF3. */
  static constexpr uint8_t kCaptureFlagBit{ICF3};
  /*! \brief The number of channels: A, B and C. */
  static constexpr uint8_t kChannelCount{3u};

  /*!
   * \brief The compare output of a channel: its pin, its bits and its register.
   * \tparam Which the channel, Channel::kA, Channel::kB or Channel::kC
   */
  template <Channel Which>
  struct CompareOutput;

  /*! \brief TCCR3A. */
  static auto Tccra() -> decltype((TCCR3A)) { return TCCR3A; }
  /*! \brief TCCR3B. */
  static auto Tccrb() -> decltype((TCCR3B)) { return TCCR3B; }
  /*! \brief ICR3. */
  static auto Icr() -> decltype((ICR3)) { return ICR3; }
  /*! \brief TIMSK3. */
  static auto Timsk() -> decltype((TIMSK3)) { return TIMSK3; }
  /*! \brief TIFR3. */
  static auto Tifr() -> decltype((TIFR3)) { return TIFR3; }
  /*! \brief TCNT3. */
  static auto Tcnt() -> decltype((TCNT3)) { return TCNT3; }
};

/*! \brief OC3A, channel A of Timer3. */
template <>
struct Timer3Registers::CompareOutput<Channel::kA> {
  /*! \brief The Arduino pin of OC3A, PE3: 5 on the Mega. */
  static constexpr uint8_t kPin{5};
  /*! \brief The bit of TCCR3A that connects OC3A: COM3A1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM3A1};
  /*! \brief The bit of TCCR3A that toggles OC3A at each match: COM3A0. */
  static constexpr uint8_t kToggleBit{COM3A0};
  /*! \brief OCR3A, its compare value. */
  static auto Compare() -> decltype((OCR3A)) { return OCR3A; }
};

/*! \brief OC3B, channel B of Timer3. */
template <>
struct Timer3Registers::CompareOutput<Channel::kB> {
  /*! \brief The Arduino pin of OC3B, PE4: 2 on the Mega. */
  static constexpr uint8_t kPin{2};
  /*! \brief The bit of TCCR3A that connects OC3B: COM3B1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM3B1};
  /*! \brief The bit of TCCR3A that toggles OC3B at each match: COM3B0. */
  static constexpr uint8_t kToggleBit{COM3B0};
  /*! \brief OCR3B, its compare value. */
  static auto Compare() -> decltype((OCR3B)) { return OCR3B; }
};

/*! \brief OC3C, channel C of Timer3. */
template <>
struct Timer3Registers::CompareOutput<Channel::kC> {
  /*! \brief The Arduino pin of OC3C, PE5: 3 on the Mega. */
  static constexpr uint8_t kPin{3};
  /*! \brief The bit of TCCR3A that connects OC3C: COM3C1, non-inverting. */
  static constexpr uint8_t kCompareOutputBit{COM3C1};
  /*! \brief The bit of TCCR3A that toggles OC3C at each match: COM3C0. */
  static constexpr uint8_t kToggleBit{COM3C0};
  /*! \brief OCR3C, its compare value. */
  static auto Compare() -> decltype((OCR3C)) { return OCR3C; }
};

/*!
 * \brief Timer/Counter3 of the ATmega2560, for the modes of a timer: its
 *        registers, its prescalers and its channels.
 */
using Timer3Traits = Timer16Traits<Timer3Registers>;

namespace internal {

/*! \cond */
// Defined by timer3.cpp and by timer3_ctc.cpp alike: a sketch that uses
// timer3 and timer3_ctc fails to link, with this name in the linker's
// message, rather than have two objects drive one timer.
extern uint8_t const kOneObjectOnTimer3;
/*! \endcond */

}  // namespace internal

}  // namespace atmega_timer

#endif  // __AVR_ATmega2560__

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER3_TRAITS_H_
