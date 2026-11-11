// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "atmega_timer/Utility/array.h"

#include <gtest/gtest.h>
#include <stdint.h>

namespace atmega_timer::internal {
namespace {

TEST(ArrayTest, HasTheSizeItIsMadeWith) {
  array<uint16_t, 3> const values{};

  EXPECT_EQ(values.size(), 3U);
}

TEST(ArrayTest, HasItsSizeWhenCompiled) {
  constexpr array<uint16_t, 2> kValues{};

  static_assert(kValues.size() == 2U, "the size is known when compiled");
}

TEST(ArrayTest, IsZeroWhenValueInitialized) {
  array<uint16_t, 2> const values{};

  EXPECT_EQ(values[0], 0U);
  EXPECT_EQ(values[1], 0U);
}

TEST(ArrayTest, IsInitializedAsACArray) {
  array<uint16_t, 3> const values{{7, 8, 9}};

  EXPECT_EQ(values[0], 7U);
  EXPECT_EQ(values[1], 8U);
  EXPECT_EQ(values[2], 9U);
}

TEST(ArrayTest, WritesAnElementByIndexAndLeavesTheOthers) {
  array<uint16_t, 3> values{};

  values[1] = 1023U;

  EXPECT_EQ(values[0], 0U);
  EXPECT_EQ(values[1], 1023U);
  EXPECT_EQ(values[2], 0U);
}

}  // namespace
}  // namespace atmega_timer::internal
