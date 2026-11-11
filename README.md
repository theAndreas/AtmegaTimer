# AtmegaTimer

A periodic interrupt, a PWM on every channel a timer leaves free, and a time readout, on the
16-bit Timer1 and the 8-bit Timer2 of the ATmega328P, as on the Arduino Uno and Nano. It
continues [TimerTwo](https://github.com/theAndreas/TimerTwo) 2.0.0, which had Timer2 alone.

AtmegaTimer is in development and not yet in the Library Manager. Timer3 to Timer5 of the
ATmega2560 of the Mega, and CTC as a second mode, come before 1.0.0;
[ROADMAP.md](https://github.com/theAndreas/AtmegaTimer/blob/main/ROADMAP.md) says what comes in
which order.

## The timers

| Timer | Mode | PWM on | Longest period at 16 MHz | Takes from the core |
|---|---|---|---|---|
| `timer1` | phase and frequency correct PWM, ICR1 the top | pin 9 (A), pin 10 (B) | 8388607 us, about 8.4 s | `Servo`, `analogWrite()` on pins 9 and 10 |
| `timer2` | phase correct PWM, OCR2A the top | pin 3 (B) | 32767 us | `tone()`, `analogWrite()` on pins 3 and 11 |

A sketch links only the timers it uses, with their interrupts: one on Timer2 leaves Timer1 to
`Servo`. Each channel has a duty of its own, from 0, always low, to 1023, always high, the same
on every timer. A new period keeps the share of each duty; on Timer1 it takes effect at the next
bottom of the counter, so that no pulse of a PWM breaks off.

## Installing

Clone it into the `libraries` folder of your sketchbook, under the name `AtmegaTimer`:

```bash
git clone https://github.com/theAndreas/AtmegaTimer ~/Arduino/libraries/AtmegaTimer
```

It is built and checked for `arduino:avr:uno`.

## Using it

```cpp
#include <AtmegaTimer.h>

void OnTick() {
  // every millisecond, in the interrupt: keep it short
}

void setup() {
  atmega_timer::Discard(atmega_timer::timer2.Begin(1000, OnTick));
  // Half of the period high: 512 of atmega_timer::kDutyMax, 1023.
  atmega_timer::Discard(
      atmega_timer::timer2.EnablePwm<atmega_timer::Channel::kB>(512));
  atmega_timer::Discard(atmega_timer::timer2.Start());
}

void loop() {}
```

A call that can refuse returns a `Status`, which says why; `Discard()` says that a sketch does
not care. A channel the timer does not have does not compile: Timer2 has channel B alone, as
OCR2A is its top. [examples/Basics](examples/Basics/Basics.ino) is the sketch above,
[examples/Read](examples/Read/Read.ino) checks each call and reads the time, and
[examples/TwoChannels](examples/TwoChannels/TwoChannels.ino) drives both PWMs of Timer1 and
changes their period while they run. The reference is built from the Doxygen blocks of `src/`.

## From TimerTwo 2.0.0

| TimerTwo 2.0.0 | AtmegaTimer |
|---|---|
| `#include <TimerTwo.h>`, `timer_two::` | `#include <AtmegaTimer.h>`, `atmega_timer::` |
| `EnablePwm(duty)`, `SetPwmDuty(duty)`, `DisablePwm()` | `EnablePwm<Channel::kB>(duty)` and so on: the channel is named |
| A duty of 0 to 255 | A duty of 0 to 1023 |
| `timer_two::TimeType` | `atmega_timer::Timer2::TimeType`, each timer its own |

The rest stays: `Begin()`, `Start()`, `Stop()`, `Resume()`, `SetPeriod()`, `Read()`, the
callback, `Status` and its errors. A callback attached while the timer is stopped is called after
`Resume()` now, as in TimerTwo's last release.

## Developing

The dev container in [.devcontainer/](.devcontainer/README.md) carries every tool. `make` lists
every build and check, the same CI runs:

```bash
make setup    # the pinned Arduino core, once
make lint     # the checks of seconds; make fmt formats
make tests    # the tests on the host
make check    # all of it, warnings as errors, as CI
```

A table of the examples' flash and RAM in this README, between the lines `<!-- sizes -->` and
`<!-- /sizes -->`, is checked against the build: `make check` fails where it differs from what
`make examples` measures, and `make sizes FIX=1` writes the measured table in.

The checks, the CI and the dev container come from the
[template](https://github.com/theAndreas/embedded-cpp-template); `uv run copier update` brings
its newest version. What is the project's own for them lies in `tools/project.sh` and
`tools/project.mk`.

## Licence

GPL-3.0-or-later, see [LICENSE](LICENSE).
