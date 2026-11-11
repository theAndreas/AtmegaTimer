# AtmegaTimer

Periodic interrupt, PWM and time readout on Timer1 to Timer5 of the ATmega328P and ATmega2560.

## Installing

Clone it into the `libraries` folder of your sketchbook, under the name `AtmegaTimer`:

```bash
git clone https://github.com/theAndreas/AtmegaTimer ~/Arduino/libraries/AtmegaTimer
```

It is built and checked for `arduino:avr:uno`.

## Using it

```cpp
#include <AtmegaTimer.h>

void setup() { Serial.begin(115200); }

void loop() {
  Serial.println(atmega_timer::Clamp(analogRead(A0), 100, 900));
  delay(1000);
}
```

The examples in [examples/](examples) show more; the reference is built from the Doxygen blocks
of `src/`, see [docs/Documentation.md](docs/Documentation.md).

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
