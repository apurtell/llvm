//===- unittests/Support/SwapByteOrderTest.cpp - swap byte order test -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"
#include "llvm/Support/SwapByteOrder.h"
#include <cstdlib>
#include <ctime>
using namespace llvm;

#undef max

namespace {

// In these first two tests all of the original_uintx values are truncated
// except for 64. We could avoid this, but there's really no point.

TEST(getSwappedBytes, UnsignedRoundTrip) {
  // The point of the bit twiddling of magic is to test with and without bits
  // in every byte.
  uint64_t value = 1;
  for (std::size_t i = 0; i <= sizeof(value); ++i) {
    uint8_t original_uint8 = static_cast<uint8_t>(value);
    EXPECT_EQ(original_uint8,
              sys::getSwappedBytes(sys::getSwappedBytes(original_uint8)));

    uint16_t original_uint16 = static_cast<uint16_t>(value);
    EXPECT_EQ(original_uint16,
              sys::getSwappedBytes(sys::getSwappedBytes(original_uint16)));

    uint32_t original_uint32 = static_cast<uint32_t>(value);
    EXPECT_EQ(original_uint32,
              sys::getSwappedBytes(sys::getSwappedBytes(original_uint32)));

    uint64_t original_uint64 = static_cast<uint64_t>(value);
    EXPECT_EQ(original_uint64,
              sys::getSwappedBytes(sys::getSwappedBytes(original_uint64)));

    value = (value << 8) | 0x55; // binary 0101 0101.
  }
}

TEST(getSwappedBytes, SignedRoundTrip) {
  // The point of the bit twiddling of magic is to test with and without bits
  // in every byte.
  uint64_t value = 1;
  for (std::size_t i = 0; i <= sizeof(value); ++i) {
    int8_t original_int8 = static_cast<int8_t>(value);
    EXPECT_EQ(original_int8,
              sys::getSwappedBytes(sys::getSwappedBytes(original_int8)));

    int16_t original_int16 = static_cast<int16_t>(value);
    EXPECT_EQ(original_int16,
              sys::getSwappedBytes(sys::getSwappedBytes(original_int16)));

    int32_t original_int32 = static_cast<int32_t>(value);
    EXPECT_EQ(original_int32,
              sys::getSwappedBytes(sys::getSwappedBytes(original_int32)));

    int64_t original_int64 = static_cast<int64_t>(value);
    EXPECT_EQ(original_int64,
              sys::getSwappedBytes(sys::getSwappedBytes(original_int64)));

    // Test other sign.
    value *= -1;

    original_int8 = static_cast<int8_t>(value);
    EXPECT_EQ(original_int8,
              sys::getSwappedBytes(sys::getSwappedBytes(original_int8)));

    original_int16 = static_cast<int16_t>(value);
    EXPECT_EQ(original_int16,
              sys::getSwappedBytes(sys::getSwappedBytes(original_int16)));

    original_int32 = static_cast<int32_t>(value);
    EXPECT_EQ(original_int32,
              sys::getSwappedBytes(sys::getSwappedBytes(original_int32)));

    original_int64 = static_cast<int64_t>(value);
    EXPECT_EQ(original_int64,
              sys::getSwappedBytes(sys::getSwappedBytes(original_int64)));

    // Return to normal sign and twiddle.
    value *= -1;
    value = (value << 8) | 0x55; // binary 0101 0101.
  }
}

TEST(getSwappedBytes, uint8_t) {
  EXPECT_EQ(uint8_t(0x11), sys::getSwappedBytes(uint8_t(0x11)));
}

TEST(getSwappedBytes, uint16_t) {
  EXPECT_EQ(uint16_t(0x1122), sys::getSwappedBytes(uint16_t(0x2211)));
}

TEST(getSwappedBytes, uint32_t) {
  EXPECT_EQ(uint32_t(0x11223344), sys::getSwappedBytes(uint32_t(0x44332211)));
}

TEST(getSwappedBytes, uint64_t) {
  EXPECT_EQ(uint64_t(0x1122334455667788ULL),
    sys::getSwappedBytes(uint64_t(0x8877665544332211ULL)));
}

TEST(getSwappedBytes, int8_t) {
  EXPECT_EQ(int8_t(0x11), sys::getSwappedBytes(int8_t(0x11)));
}

TEST(getSwappedBytes, int16_t) {
  EXPECT_EQ(int16_t(0x1122), sys::getSwappedBytes(int16_t(0x2211)));
}

TEST(getSwappedBytes, int32_t) {
  EXPECT_EQ(int32_t(0x11223344), sys::getSwappedBytes(int32_t(0x44332211)));
}

TEST(getSwappedBytes, int64_t) {
  EXPECT_EQ(int64_t(0x1122334455667788LL),
    sys::getSwappedBytes(int64_t(0x8877665544332211LL)));
}

TEST(swapByteOrder, uint8_t) {
  uint8_t value = 0x11;
  sys::swapByteOrder(value);
  EXPECT_EQ(uint8_t(0x11), value);
}

TEST(swapByteOrder, uint16_t) {
  uint16_t value = 0x2211;
  sys::swapByteOrder(value);
  EXPECT_EQ(uint16_t(0x1122), value);
}

TEST(swapByteOrder, uint32_t) {
  uint32_t value = 0x44332211;
  sys::swapByteOrder(value);
  EXPECT_EQ(uint32_t(0x11223344), value);
}

TEST(swapByteOrder, uint64_t) {
  uint64_t value = 0x8877665544332211ULL;
  sys::swapByteOrder(value);
  EXPECT_EQ(uint64_t(0x1122334455667788ULL), value);
}

TEST(swapByteOrder, int8_t) {
  int8_t value = 0x11;
  sys::swapByteOrder(value);
  EXPECT_EQ(int8_t(0x11), value);
}

TEST(swapByteOrder, int16_t) {
  int16_t value = 0x2211;
  sys::swapByteOrder(value);
  EXPECT_EQ(int16_t(0x1122), value);
}

TEST(swapByteOrder, int32_t) {
  int32_t value = 0x44332211;
  sys::swapByteOrder(value);
  EXPECT_EQ(int32_t(0x11223344), value);
}

TEST(swapByteOrder, int64_t) {
  int64_t value = 0x8877665544332211LL;
  sys::swapByteOrder(value);
  EXPECT_EQ(int64_t(0x1122334455667788LL), value);
}

}
