// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// avr-libc's ISR(), for the tests on the host: the handler is a function of C
// linkage by the vector's name, which a test calls in place of the
// interrupt.

#ifndef ATMEGATIMER_TEST_MOCK_AVR_INTERRUPT_H_
#define ATMEGATIMER_TEST_MOCK_AVR_INTERRUPT_H_

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ISR(vector) extern "C" void vector()

// The handlers of the overflows of Timer1 and Timer2, of the input capture of
// Timer1 and of the compare match A of Timer2, which a test calls.
extern "C" void TIMER1_OVF_vect();    // NOLINT(readability-identifier-naming)
extern "C" void TIMER2_OVF_vect();    // NOLINT(readability-identifier-naming)
extern "C" void TIMER1_CAPT_vect();   // NOLINT(readability-identifier-naming)
extern "C" void TIMER2_COMPA_vect();  // NOLINT(readability-identifier-naming)
#ifdef __AVR_ATmega2560__
// The same of Timer3 to Timer5, which the ATmega2560 alone has.
extern "C" void TIMER3_OVF_vect();   // NOLINT(readability-identifier-naming)
extern "C" void TIMER3_CAPT_vect();  // NOLINT(readability-identifier-naming)
extern "C" void TIMER4_OVF_vect();   // NOLINT(readability-identifier-naming)
extern "C" void TIMER4_CAPT_vect();  // NOLINT(readability-identifier-naming)
extern "C" void TIMER5_OVF_vect();   // NOLINT(readability-identifier-naming)
extern "C" void TIMER5_CAPT_vect();  // NOLINT(readability-identifier-naming)
#endif

#endif  // ATMEGATIMER_TEST_MOCK_AVR_INTERRUPT_H_
