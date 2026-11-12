// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Checks the timers where only the AVR shows it: the callbacks of a period,
// a new period while a timer runs, the time read, and what a channel puts out.
// Each check prints a line, pass or FAIL, and the run ends with the number
// that failed; SelfCheck*.expected is what a correct run prints.
//
// Three builds, as a sketch takes one object of a timer:
// - By default, as tools/self_check.sh runs it in simavr: timer2_ctc alone,
//   the one timer and mode simavr counts as the AVR does (ROADMAP.md, A-7).
// - With ATMEGATIMER_ON_A_BOARD, as tools/board_check.sh builds it for the
//   Uno: timer1_ctc as well, and the toggle of each, read back on a pin
//   wired to it.
// - With ATMEGATIMER_PWM beside it: timer1 and timer2 in phase correct PWM in
//   place of CTC, and the PWM of each.
//
// On a board, two wires: pin 9, OC1A, to pin 2, and pin 3, OC2B, to pin 4.

#include <AtmegaTimer.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <util/atomic.h>

#if defined(ATMEGATIMER_PWM) && !defined(ATMEGATIMER_ON_A_BOARD)
#error "simavr does not count phase correct PWM: the PWM build is for a board"
#endif

namespace {

using atmega_timer::Status;

constexpr uint32_t kBaudRate{115200};

// The period of the checks, and the one SetPeriod() sets while it runs, in
// microseconds.
constexpr uint16_t kPeriod{1000};
constexpr uint16_t kShorterPeriod{500};
// How long the callbacks are counted: 100 periods.
constexpr uint32_t kWindow{100000};
// Long enough for the counter to take a period that waits for it.
constexpr uint32_t kSettle{2000};

// A range of what a check accepts, both ends in it.
struct Range {
  uint32_t lowest;
  uint32_t highest;
};

// One callback more or less at the ends of the window.
constexpr Range kCallsInTheWindow{99, 101};
constexpr Range kCallsAtTheShorterPeriod{199, 201};

uint8_t failed{0};

// Written by the interrupt, read here in a critical section: 16 bits.
uint16_t volatile calls{0};

void CountCall() { ++calls; }

uint16_t Calls() {
  uint16_t value{0};
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = calls; }
  return value;
}

void ResetCalls() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { calls = 0; }
}

void Wait(uint32_t microseconds) {
  uint32_t const start{micros()};
  while (micros() - start < microseconds) {
  }
}

bool IsIn(uint32_t value, Range range) {
  return (value >= range.lowest) && (value <= range.highest);
}

// Prints the line of a check; a failed one with what it got.
void Report(bool passed, __FlashStringHelper const* timer,
            __FlashStringHelper const* check, uint32_t got) {
  Serial.print(passed ? F("pass: ") : F("FAIL: "));
  Serial.print(timer);
  Serial.print(' ');
  Serial.print(check);
  if (!passed) {
    ++failed;
    Serial.print(F(", got "));
    Serial.print(got);
  }
  Serial.println();
}

// A call that must not be refused; a refusal is a check that failed.
void Expect(Status status, __FlashStringHelper const* timer,
            __FlashStringHelper const* call) {
  if (status.IsOk()) {
    return;
  }
  ++failed;
  Serial.print(F("FAIL: "));
  Serial.print(timer);
  Serial.print(' ');
  Serial.print(call);
  Serial.print(F(" refused: "));
  Serial.println(status.GetErrorText());
}

// A callback once a period, and a time read within the period.
template <typename Timer>
void CheckCallbacks(Timer& timer, __FlashStringHelper const* name) {
  ResetCalls();
  Expect(timer.Start(), name, F("Start()"));
  Wait(kWindow);
  uint16_t const counted{Calls()};
  atmega_timer::Result<typename Timer::TimeType> const time{timer.Read()};
  Expect(timer.Stop(), name, F("Stop()"));

  Report(IsIn(counted, kCallsInTheWindow), name,
         F("calls back 100 times in 100 ms"), counted);
  bool const within{time.IsOk() && (time.GetValue() < kPeriod)};
  Report(within, name, F("reads a time within the period"), time.GetValue());
}

// A period set while the timer runs, which it takes a period later at most.
template <typename Timer>
void CheckNewPeriod(Timer& timer, __FlashStringHelper const* name) {
  Expect(timer.Start(), name, F("Start()"));
  Wait(kWindow / 4U);
  Expect(timer.SetPeriod(kShorterPeriod), name, F("SetPeriod()"));
  Wait(kSettle);
  ResetCalls();
  Wait(kWindow);
  uint16_t const counted{Calls()};
  Expect(timer.Stop(), name, F("Stop()"));
  Expect(timer.SetPeriod(kPeriod), name, F("SetPeriod()"));

  Report(IsIn(counted, kCallsAtTheShorterPeriod), name,
         F("calls back 200 times in 100 ms after a new period"), counted);
}

#ifdef ATMEGATIMER_ON_A_BOARD
using atmega_timer::Channel;

// The pins the outputs are wired to.
constexpr uint8_t kTimer1Input{2};
constexpr uint8_t kTimer2Input{4};

// How long pulseIn() waits for a pulse: no wire, no pulse.
constexpr uint32_t kPulseTimeout{10000};
// pulseIn() counts in a loop, which the interrupt of millis() holds up for a
// few microseconds: 2 % of a period.
constexpr uint32_t kPulseTolerance{20};

#ifdef ATMEGATIMER_PWM
// A quarter of the period high: 256 / 1023 of the top.
constexpr uint16_t kDuty{256};
constexpr uint32_t kHighTime{250};
constexpr uint32_t kLowTime{750};
#else
// A toggle is high for one period and low for the next.
constexpr uint32_t kHighTime{kPeriod};
constexpr uint32_t kLowTime{kPeriod};
#endif

Range Around(uint32_t value) {
  return Range{value - kPulseTolerance, value + kPulseTolerance};
}

#ifdef ATMEGATIMER_PWM
Status EnableOutput(atmega_timer::Timer1& timer) {
  return timer.EnablePwm<Channel::kA>(kDuty);
}

Status EnableOutput(atmega_timer::Timer2& timer) {
  return timer.EnablePwm<Channel::kB>(kDuty);
}

void DisableOutput(atmega_timer::Timer1& timer) {
  timer.DisablePwm<Channel::kA>();
}

void DisableOutput(atmega_timer::Timer2& timer) {
  timer.DisablePwm<Channel::kB>();
}
#else
Status EnableOutput(atmega_timer::Timer1Ctc& timer) {
  return timer.EnableToggle<Channel::kA>();
}

Status EnableOutput(atmega_timer::Timer2Ctc& timer) {
  return timer.EnableToggle<Channel::kB>();
}

void DisableOutput(atmega_timer::Timer1Ctc& timer) {
  timer.DisableToggle<Channel::kA>();
}

void DisableOutput(atmega_timer::Timer2Ctc& timer) {
  timer.DisableToggle<Channel::kB>();
}
#endif

// What the channel puts out, read back on the pin wired to it, without the
// callback, whose interrupt would hold up pulseIn() as well.
template <typename Timer>
void CheckOutput(Timer& timer, __FlashStringHelper const* name, uint8_t input) {
  pinMode(input, INPUT);
  timer.DetachInterrupt();
  Expect(EnableOutput(timer), name, F("its output"));
  Expect(timer.Start(), name, F("Start()"));
  uint32_t const high{pulseIn(input, HIGH, kPulseTimeout)};
  uint32_t const low{pulseIn(input, LOW, kPulseTimeout)};
  Expect(timer.Stop(), name, F("Stop()"));
  DisableOutput(timer);

  Report(IsIn(high, Around(kHighTime)), name, F("puts out the high time"),
         high);
  Report(IsIn(low, Around(kLowTime)), name, F("puts out the low time"), low);
}
#endif

template <typename Timer>
void CheckTimer(Timer& timer, __FlashStringHelper const* name) {
  Expect(timer.Begin(kPeriod, CountCall), name, F("Begin()"));
  CheckCallbacks(timer, name);
  CheckNewPeriod(timer, name);
}

// Ends the run so that simavr ends too: asleep with the interrupts off.
void End() {
  Serial.print(F("failed: "));
  Serial.println(failed);
  Serial.flush();
  cli();
  sleep_mode();
}

}  // namespace

void setup() {
  Serial.begin(kBaudRate);
#ifdef ATMEGATIMER_PWM
  CheckTimer(atmega_timer::timer2, F("timer2"));
  CheckTimer(atmega_timer::timer1, F("timer1"));
  CheckOutput(atmega_timer::timer2, F("timer2"), kTimer2Input);
  CheckOutput(atmega_timer::timer1, F("timer1"), kTimer1Input);
#else
  CheckTimer(atmega_timer::timer2_ctc, F("timer2_ctc"));
#ifdef ATMEGATIMER_ON_A_BOARD
  CheckTimer(atmega_timer::timer1_ctc, F("timer1_ctc"));
  CheckOutput(atmega_timer::timer2_ctc, F("timer2_ctc"), kTimer2Input);
  CheckOutput(atmega_timer::timer1_ctc, F("timer1_ctc"), kTimer1Input);
#endif
#endif
  End();
}

void loop() {}
