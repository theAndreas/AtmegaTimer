// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/attributes.h"

#include <gtest/gtest.h>

namespace atmega_timer {
namespace {

// A result that must not be dropped, and an effect that shows the call.
class Sender {
 public:
  ATMEGATIMER_NODISCARD bool Send() {
    sent_ = true;
    return true;
  }

  ATMEGATIMER_NODISCARD bool sent() const { return sent_; }

 private:
  bool sent_{false};
};

// That Discard() silences the warning is checked by the build itself, with
// warnings as errors under `make check`: before C++17 GCC warns through a
// cast to void.
TEST(DiscardTest, LeavesTheCallItsEffect) {
  Sender sender;

  Discard(sender.Send());

  EXPECT_TRUE(sender.sent());
}

}  // namespace
}  // namespace atmega_timer
