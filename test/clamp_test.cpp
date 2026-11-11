// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/clamp.h"

#include <gtest/gtest.h>

namespace atmega_timer {
namespace {

TEST(MinTest, ReturnsTheLesser) {
  EXPECT_EQ(Min(1, 2), 1);
  EXPECT_EQ(Min(2, 1), 1);
}

TEST(MaxTest, ReturnsTheGreater) {
  EXPECT_EQ(Max(1, 2), 2);
  EXPECT_EQ(Max(2, 1), 2);
}

TEST(ClampTest, KeepsAValueInRange) { EXPECT_EQ(Clamp(5, 0, 10), 5); }

TEST(ClampTest, LimitsAValueBelow) { EXPECT_EQ(Clamp(-3, 0, 10), 0); }

TEST(ClampTest, LimitsAValueAbove) { EXPECT_EQ(Clamp(42, 0, 10), 10); }

TEST(ClampTest, KeepsTheBounds) {
  EXPECT_EQ(Clamp(0, 0, 10), 0);
  EXPECT_EQ(Clamp(10, 0, 10), 10);
}

TEST(ClampTest, IsAConstantExpression) {
  constexpr int kHigh{10};
  constexpr int kAbove{42};
  static_assert(Clamp(kAbove, 0, kHigh) == kHigh, "clamped at compile time");
}

}  // namespace
}  // namespace atmega_timer
