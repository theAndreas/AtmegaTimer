# Roadmap

Where AtmegaTimer is going: what is done, what comes next, and why. The items carry an id, so
that a commit, a pull request or a discussion can name one; an id never changes its meaning, and a
new item takes the next free number. A pull request that works on an item sets its status in the
same PR; an item is done once that PR is merged.

## Milestones

### M1 - AtmegaTimer 1.0.0 in the Library Manager

Timer1 and Timer2 of the ATmega328P, and Timer1 to Timer5 of the ATmega2560, each with a periodic
interrupt, a PWM on every channel its mode leaves free, and a time readout, in phase correct PWM
and in CTC.

| Id | Item | Status |
|---|---|---|
| A-1 | The core of TimerTwo 2.0.0, with Timer2 in phase correct PWM: `src/timer_two/` of TimerTwo 2.0.0 in `src/atmega_timer/`, the namespace `atmega_timer`, the mock of the registers and the tests with it; the commit names the commit of TimerTwo it takes. The skeleton of the template, `clamp.h` and its test, goes. The duty becomes 0 to 1023. *Done when* the tests of TimerTwo pass here, with the duties scaled, at 1, 16 and 20 MHz | open |
| A-2 | Traits in place of Registers, after A-1: `Timer<Registers>` assumes 8 bits and the bits of Timer/Counter2. The traits of a timer say its width, its prescalers (Timer2: 1, 8, 32, 64, 128, 256, 1024; the others: 1, 8, 64, 256, 1024), its bits of the mode, its top register, its channels with their pins, and its `TimeType`: the longest period of a 16-bit timer, about 8.4 s at 16 MHz, needs 32 bits. The core and the modes take what differs from the traits only. *Done when* Timer2 runs on its traits, and the tests of A-1 pass unchanged | open |
| A-3 | Timer1 in phase and frequency correct PWM, every channel, after A-2: mode 8, with ICR1 as the top, whose compare values are taken at the bottom, not at the top as in mode 10, so the PWM stays symmetric when the period changes. ICR1 itself is not buffered: a top written below TCNT1 lets the counter run on to 0xFFFF, so `SetPeriod()` while it runs has to avoid that, which this item decides how. TOV1 comes at the bottom, as TOV2 in mode 5. The old TimerOne serves as a reference only; the bugs of TimerTwo's OIL 2 are in it too. *Done when* the tests of Timer1 cover what those of Timer2 cover, and each channel's duty is set on its own | open |
| A-4 | The ATmega2560 beside the ATmega328P, after A-2: the template compiles the examples for one board, `FQBN`; a list of boards, and the CI over it, belongs in the template, so that Utl and MaxMatrix get it too, and comes here by `copier update`. Until then `tools/project.sh` lists the Mega here. The mock of `avr/io.h` and the tests get a build per MCU. On the Mega the pins differ: OC1A to OC1C are 11, 12 and 13, OC2B is 9. *Done when* `make check` compiles the examples for `arduino:avr:uno` and `arduino:avr:mega`, and the tests run for both | open |
| A-5 | Timer3, Timer4 and Timer5 of the ATmega2560, after A-3 and A-4: their traits and objects, only where `__AVR_ATmega2560__` is defined. Pins: Timer3 5, 2, 3; Timer4 6, 7, 8; Timer5 46, 45, 44. `Servo` takes Timer5 first on the Mega, then 1, 3 and 4; the README says so | open |
| A-6 | CTC for every timer, after A-3: `Ctc<Traits>`, the period counted once rather than up and down, so twice the resolution and half the longest period, and `Read()` without the wait of a step to find the direction. Timer2 in mode 2 with OCR2A as the top, the others in mode 12 with ICRn. Whether a channel can give the period as a toggle, at half the frequency, is decided here | open |
| A-7 | SelfCheck in simavr, after A-6: `examples/SelfCheck`, the period counted in callbacks and, where simavr models it, the PWM measured. simavr counts Timer2 in CTC and fast PWM but not in phase correct PWM (TimerTwo OIL 5, simavr #442); whether it models Timer1 in mode 8 is measured here first. Until then SelfCheck runs CTC only | open |
| A-8 | TimerTwo points to AtmegaTimer, after A-9: TimerTwo 2.0.0 does not go to the Library Manager, its OIL 4 is dropped, its README points here, and the repository is archived | open |
| A-9 | Release 1.0.0 and the Library Manager, after A-5 and A-6: the README with the API, the timers and their pins per board, what each timer takes from the core and from other libraries, and the table of flash and RAM between `<!-- sizes -->` markers; the tag, `release.yml`, and the request to the Arduino Library Manager, under the name AtmegaTimer, free on 2026-11-11 | open |

## Decisions

- **AtmegaTimer replaces two projects** (2026-11-11):
  [TimerTwo](https://github.com/theAndreas/TimerTwo), whose 2.0.0 is the starting point, and
  AndreasBur/Timer, the TimerOne and TimerTwo of 2016 in AtmelStudio projects, deleted on
  2026-11-11 and kept by the author as a git bundle. Its name follows
  [XmegaTimer](https://github.com/theAndreas/XmegaTimer): ATxmega there, ATmega here.
- **The classic ATmega**: the ATmega328P (Uno, Nano) and the ATmega2560 (Mega),
  `architectures=avr`. Not the ATmega4809 of the Nano Every, whose TCA and TCB are other timers.
- **Timer0 is left out**: the core's `millis()` runs on it.
- **The modes are class templates of their own on one core**, `Ctc<Traits>` and
  `PhaseCorrectPwm<Traits>`; a timer does not change its mode while it runs, as the period would
  change its meaning.
- **Every channel a mode leaves free gives a PWM of its own duty**: on Timer1, 3, 4 and 5 the top
  is ICRn, so A, B and, on the ATmega2560, C are free; on Timer2 the top is OCR2A, so only B.
- **The channel is a parameter of the template**, `EnablePwm<Channel::kB>(duty)`: a channel the
  timer does not have does not compile.
- **The duty is a `uint16_t` from 0 to 1023 on every timer**, as the TimerOne of PJRC has it: 0
  keeps the output low, 1023 high. A sketch moves to another timer without changing its duties.
- **Each timer's object and interrupt lie in a source file of their own**, so that the linker
  takes only the vectors of the timers a sketch uses, and a sketch on Timer2 keeps `Servo` on
  Timer1.
- **`Status`, `Error` and `Result` as in TimerTwo 2.0.0**, and with them its state machine and its
  names.
