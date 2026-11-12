#!/bin/bash
#
# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs examples/SelfCheck on a board at a serial port and compares what it prints with what it
# should: in a build for CTC and one for PWM, as a sketch takes one object of a timer, each
# against SelfCheck.board-<build>.expected. The board is wired as the sketch says at its top.
# The project's own script, which `make board-check PORT=...` runs; CI has no board.
#
#   tools/board_check.sh /dev/ttyACM0

set -euo pipefail

# shellcheck source=tools/common.sh
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

readonly EXAMPLE_DIR="${ROOT_DIR}/examples/${SELF_CHECK_EXAMPLE}"
readonly BAUD_RATE=115200
# A run prints its lines within a few seconds of the reset; a line that does not come in this
# time will not come.
readonly LINE_TIMEOUT_SECONDS=10
readonly BUILDS=(ctc pwm)

# Prints the compiler flags of a build of the sketch.
flags_of() {
  case "$1" in
    ctc) echo "-DATMEGATIMER_ON_A_BOARD" ;;
    pwm) echo "-DATMEGATIMER_ON_A_BOARD -DATMEGATIMER_PWM" ;;
    *)
      err "No build $1."
      return 1
      ;;
  esac
}

# Compiles the sketch in a build and uploads it to the board at a port.
upload() {
  local build="$1"
  local port="$2"
  local build_dir="${BUILD_DIR}/board/${build}"
  local flags
  flags="$(flags_of "${build}")"
  arduino-cli compile --fqbn "${FQBN}" --library "${ROOT_DIR}" --warnings all \
    --build-path "${build_dir}" --build-property "compiler.cpp.extra_flags=${flags}" \
    "${EXAMPLE_DIR}" > /dev/null
  arduino-cli upload --fqbn "${FQBN}" --port "${port}" --input-dir "${build_dir}" > /dev/null
}

# Prints the lines of the checks the board sends, up to the one of the number that failed.
# Opening the port resets the board, so the run starts over and is read from its first line.
read_run() {
  local port="$1"
  local fd
  local line
  stty -F "${port}" "${BAUD_RATE}" raw -echo
  exec {fd}< "${port}"
  while IFS= read -r -t "${LINE_TIMEOUT_SECONDS}" -u "${fd}" line; do
    line="${line%$'\r'}"
    case "${line}" in
      pass:* | FAIL:*) echo "${line}" ;;
      failed:*)
        echo "${line}"
        break
        ;;
    esac
  done || true
  exec {fd}<&-
}

main() {
  if [[ $# -ne 1 || -z "$1" ]]; then
    err "Usage: $0 PORT, as /dev/ttyACM0"
    exit 2
  fi
  local port="$1"
  local failed=0
  local build
  local expected
  local actual
  for build in "${BUILDS[@]}"; do
    upload "${build}" "${port}"
    expected="${EXAMPLE_DIR}/${SELF_CHECK_EXAMPLE}.board-${build}.expected"
    actual="$(read_run "${port}")"
    if diff -u --label expected --label "${build}" "${expected}" <(echo "${actual}"); then
      echo "${SELF_CHECK_EXAMPLE} (${build}) on ${port}: as expected"
    else
      failed=1
    fi
  done
  exit "${failed}"
}

main "$@"
