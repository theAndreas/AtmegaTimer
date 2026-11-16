// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer16Traits, what the 16-bit Timer/Counters of the ATmega have
 *        alike, for the modes of a timer: their prescalers and their modes.
 *
 * Each timer hands in its registers, its bits and its channels, as
 * Timer1Registers does in timer1_traits.h.
 */

#ifndef ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER16_TRAITS_H_
#define ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER16_TRAITS_H_

#include <stdint.h>

#include "atmega_timer/Bit.h"
#include "atmega_timer/core.h"

namespace atmega_timer {

/*! \brief The values of the clock select bits CSn2..CSn0 of a 16-bit timer. */
enum class Timer16ClockSelect : uint8_t {
  kNoClock = 0, /*!< The counter stands. */
  kDivide1,     /*!< The CPU's clock. */
  kDivide8,     /*!< The CPU's clock / 8. */
  kDivide64,    /*!< The CPU's clock / 64. */
  kDivide256,   /*!< The CPU's clock / 256. */
  kDivide1024   /*!< The CPU's clock / 1024. */
};

namespace internal {

/*! \cond */
// The largest prescaler of a 16-bit timer as a shift: 1024 is 2^10.
constexpr uint8_t kTimer16ShiftLargest{10u};

// The prescaler of clock_select as a shift, 0 for none; once for every 16-bit
// timer, in timer16_traits.cpp.
uint8_t Timer16ShiftOf(Timer16ClockSelect clock_select);
/*! \endcond */

}  // namespace internal

/*!
 * \brief A 16-bit Timer/Counter of the ATmega, for the modes of a timer: its
 *        registers, its prescalers and its channels.
 *
 * PhaseCorrectPwmMode is phase and frequency correct PWM, mode 8, with ICRn as
 * its top and every channel as a PWM. Mode 8 takes the compare values at the
 * bottom, so the PWM stays symmetric when the period changes. CtcMode is CTC,
 * mode 12, with ICRn as its top, whose input capture interrupt comes at the
 * top, and every channel as a toggle.
 *
 * \tparam Registers the Timer/Counter's own, as Timer1Registers gives them:
 *                   `kWgm2` and `kWgm3`, the bits WGMn2 and WGMn3 of TCCRnB;
 *                   `kOverflowInterruptBit`, `kOverflowFlagBit`,
 *                   `kCaptureInterruptBit` and `kCaptureFlagBit`, TOIEn, TOVn,
 *                   ICIEn and ICFn; `kChannelCount`, its channels from A on;
 *                   for each channel `CompareOutput<Which>`, with `kPin`,
 *                   `kCompareOutputBit`, `kToggleBit` and `Compare()`; and
 *                   `Tccra()`, `Tccrb()`, `Icr()`, `Timsk()`, `Tifr()` and
 *                   `Tcnt()`, which return its registers as the vendor's header
 *                   declares them
 */
template <typename Registers>
struct Timer16Traits {
  /*! \brief The type of the counter and its registers: 16 bits. */
  using Counter = uint16_t;

  /*!
   * \brief A time in microseconds: 32 bits, as the longest period is about 8.4
   *        s at 16 MHz.
   */
  using TimeType = uint32_t;

  /*! \brief The values of the clock select bits. */
  using ClockSelect = Timer16ClockSelect;

  /*! \brief The largest prescaler. */
  static constexpr ClockSelect kClockSelectLargest{ClockSelect::kDivide1024};
  /*! \brief The largest prescaler as a shift: 1024 is 2^10. */
  static constexpr uint8_t kShiftLargest{internal::kTimer16ShiftLargest};

  /*!
   * \brief Returns the prescaler of `clock_select` as a shift, 0 for none.
   * \param[in] clock_select the prescaler
   * \return the power of 2 it divides by
   */
  static uint8_t ShiftOf(ClockSelect clock_select) {
    return internal::Timer16ShiftOf(clock_select);
  }

  /*! \brief The timer in phase and frequency correct PWM, mode 8, for
   * PhaseCorrectPwm. */
  struct PhaseCorrectPwmMode {
    /*! \brief TCCRnA in mode 8: no bit of the mode. */
    static constexpr uint8_t kModeTccra{0u};
    /*! \brief TCCRnB in mode 8: WGMn3, and no clock. */
    static constexpr uint8_t kModeTccrb{Bit<uint8_t>::ToMask(Registers::kWgm3)};
    /*! \brief Whether the hardware buffers the top: ICRn is not: written below
     * the counter, it lets the counter run on to 65535. */
    static constexpr bool kTopIsBuffered{false};
    /*! \brief The bit of TIMSKn that enables the overflow interrupt. */
    static constexpr uint8_t kInterruptBit{Registers::kOverflowInterruptBit};
    /*! \brief The bit of TIFRn that flags an overflow, at the bottom. */
    static constexpr uint8_t kFlagBit{Registers::kOverflowFlagBit};
    /*! \brief The first channel that gives a PWM: A, as ICRn is the top. */
    static constexpr Channel kFirstChannel{Channel::kA};
    /*! \brief The number of channels that give a PWM: all of them. */
    static constexpr uint8_t kChannelCount{Registers::kChannelCount};
  };

  /*! \brief The timer in CTC, mode 12, for Ctc. */
  struct CtcMode {
    /*! \brief TCCRnA in mode 12: no bit of the mode. */
    static constexpr uint8_t kModeTccra{0u};
    /*! \brief TCCRnB in mode 12: WGMn3 and WGMn2, and no clock. */
    static constexpr uint8_t kModeTccrb{
        Bit<uint8_t>::Combine(Bit<uint8_t>::ToMask(Registers::kWgm3),
                              Bit<uint8_t>::ToMask(Registers::kWgm2))};
    /*! \brief Whether the hardware buffers the top: ICRn is not. */
    static constexpr bool kTopIsBuffered{false};
    /*! \brief The bit of TIMSKn that enables the input capture interrupt. */
    static constexpr uint8_t kInterruptBit{Registers::kCaptureInterruptBit};
    /*! \brief The bit of TIFRn that flags the input capture, which mode 12
     * sets at the top, ICRn, rather than at the pin ICPn. */
    static constexpr uint8_t kFlagBit{Registers::kCaptureFlagBit};
    /*! \brief The first channel that toggles: A. */
    static constexpr Channel kFirstChannel{Channel::kA};
    /*! \brief The number of channels that toggle: all of them. */
    static constexpr uint8_t kChannelCount{Registers::kChannelCount};
    /*! \brief The first channel whose compare value is its own: A, as ICRn is
     * the top. */
    static constexpr Channel kFirstOwnCompare{Channel::kA};
  };

  /*!
   * \brief The compare output of a channel: its pin, its bits and its register.
   * \tparam Which the channel
   */
  template <Channel Which>
  using CompareOutput = typename Registers::template CompareOutput<Which>;

  /*! \brief TCCRnA. */
  static auto Tccra() -> decltype(Registers::Tccra()) {
    return Registers::Tccra();
  }
  /*! \brief TCCRnB. */
  static auto Tccrb() -> decltype(Registers::Tccrb()) {
    return Registers::Tccrb();
  }
  /*! \brief ICRn, the top; not buffered, unlike the compare values. */
  static auto Top() -> decltype(Registers::Icr()) { return Registers::Icr(); }
  /*! \brief TIMSKn. */
  static auto Timsk() -> decltype(Registers::Timsk()) {
    return Registers::Timsk();
  }
  /*! \brief TIFRn, whose flags are cleared by writing 1. */
  static auto Tifr() -> decltype(Registers::Tifr()) {
    return Registers::Tifr();
  }
  /*! \brief TCNTn, the counter. */
  static auto Tcnt() -> decltype(Registers::Tcnt()) {
    return Registers::Tcnt();
  }
};

}  // namespace atmega_timer

#endif  // ATMEGATIMER_SRC_ATMEGA_TIMER_TIMER16_TRAITS_H_
