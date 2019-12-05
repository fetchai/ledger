//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace fetch {
namespace math {
namespace test {

static const int64_t N{10000};

using namespace fetch::fixed_point;

TEST(FixedPointTest, Conversion_16_16)
{
  // Get raw value
  fp32_t one(1);
  fp32_t zero_point_five(0.5);
  fp32_t one_point_five(1.5);
  fp32_t two_point_five(2.5);
  fp32_t m_one_point_five(-1.5);

  EXPECT_EQ(zero_point_five.Data(), 0x08000);
  EXPECT_EQ(one.Data(), 0x10000);
  EXPECT_EQ(one_point_five.Data(), 0x18000);
  EXPECT_EQ(two_point_five.Data(), 0x28000);

  // Convert from raw value
  fp32_t two_point_five_raw(2, 0x08000);
  fp32_t m_two_point_five_raw(-2, 0x08000);
  EXPECT_EQ(two_point_five, two_point_five_raw);
  EXPECT_EQ(m_one_point_five, m_two_point_five_raw);

  // Extreme cases:
  // smallest possible double representable to a FixedPoint
  fp32_t infinitesimal(0.00002);
  // Largest fractional closest to one, representable to a FixedPoint
  fp32_t almost_one(0.99999);
  // Largest fractional closest to one, representable to a FixedPoint
  fp32_t largest_int(std::numeric_limits<int16_t>::max() - 1);

  // Smallest possible integer, increase by 2, in order to allow for the fractional part.
  // (+1 is reserved for -inf value)
  fp32_t smallest_int(std::numeric_limits<int16_t>::min() + 2);

  // Largest possible Fixed Point number.
  fp32_t largest_fixed_point = largest_int + almost_one;

  // Smallest possible Fixed Point number.
  fp32_t smallest_fixed_point = smallest_int - almost_one;

  EXPECT_EQ(infinitesimal.Data(), fp32_t::SMALLEST_FRACTION);
  EXPECT_EQ(almost_one.Data(), fp32_t::LARGEST_FRACTION);
  EXPECT_EQ(largest_int.Data(), fp32_t::MAX_INT);
  EXPECT_EQ(smallest_int.Data(), fp32_t::MIN_INT);
  EXPECT_EQ(largest_fixed_point.Data(), fp32_t::MAX);
  EXPECT_EQ(smallest_fixed_point.Data(), fp32_t::MIN);

  EXPECT_EQ(fp32_t::MIN, 0x80010001);
  EXPECT_EQ(fp32_t::MAX, 0x7ffeffff);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int32_t>::min());
  // On the other hand we expect to be less than the largest positive integer of int32_t
  EXPECT_TRUE(largest_fixed_point.Data() < std::numeric_limits<int32_t>::max());

  EXPECT_EQ(fp32_t::TOLERANCE.Data(), 0x15);
  EXPECT_EQ(fp32_t::DECIMAL_DIGITS, 4);

  double r     = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
  auto   x32   = static_cast<fp32_t>(r) * fp32_t::FP_MAX - fp32_t::FP_MAX;
  auto   x64   = static_cast<fp64_t>(x32);
  auto   x128  = static_cast<fp128_t>(x32);
  auto   x32_2 = static_cast<fp32_t>(x128);
  EXPECT_EQ(x32, x32_2);
  auto x32_3 = static_cast<fp32_t>(x64);
  EXPECT_EQ(x32, x32_3);
}

TEST(FixedPointTest, Conversion_32_32)
{
  // Get raw value
  fp64_t one(1);
  fp64_t zero_point_five(0.5);
  fp64_t one_point_five(1.5);
  fp64_t two_point_five(2.5);
  fp64_t m_one_point_five(-1.5);

  EXPECT_EQ(zero_point_five.Data(), 0x080000000);
  EXPECT_EQ(one.Data(), 0x100000000);
  EXPECT_EQ(one_point_five.Data(), 0x180000000);
  EXPECT_EQ(two_point_five.Data(), 0x280000000);

  // Convert from raw value
  fp64_t two_point_five_raw(2, 0x080000000);
  fp64_t m_two_point_five_raw(-2, 0x080000000);
  EXPECT_EQ(two_point_five, two_point_five_raw);
  EXPECT_EQ(m_one_point_five, m_two_point_five_raw);

  // Extreme cases:
  // smallest possible double representable to a FixedPoint
  fp64_t infinitesimal(0.0000000004);
  // Largest fractional closest to one, representable to a FixedPoint
  fp64_t almost_one(0.9999999998);
  // Largest fractional closest to one, representable to a FixedPoint
  fp64_t largest_int(std::numeric_limits<int32_t>::max() - 1);

  // Smallest possible integer, increase by 2, in order to allow for the fractional part.
  // (+1 is reserved for -inf value)
  fp64_t smallest_int(std::numeric_limits<int32_t>::min() + 2);

  // Largest possible Fixed Point number.
  fp64_t largest_fixed_point = largest_int + almost_one;

  // Smallest possible Fixed Point number.
  fp64_t smallest_fixed_point = smallest_int - almost_one;

  EXPECT_EQ(infinitesimal.Data(), fp64_t::SMALLEST_FRACTION);
  EXPECT_EQ(almost_one.Data(), fp64_t::LARGEST_FRACTION);
  EXPECT_EQ(largest_int.Data(), fp64_t::MAX_INT);
  EXPECT_EQ(smallest_int.Data(), fp64_t::MIN_INT);
  EXPECT_EQ(largest_fixed_point.Data(), fp64_t::MAX);
  EXPECT_EQ(smallest_fixed_point.Data(), fp64_t::MIN);
  EXPECT_EQ(fp64_t::MIN, 0x8000000100000001);
  EXPECT_EQ(fp64_t::MAX, 0x7ffffffeffffffff);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int64_t>::min());
  // On the other hand we expect to be less than the largest positive integer of int64_t
  EXPECT_TRUE(largest_fixed_point.Data() < std::numeric_limits<int64_t>::max());

  EXPECT_EQ(fp64_t::TOLERANCE.Data(), 0x200);
  EXPECT_EQ(fp64_t::DECIMAL_DIGITS, 9);

  double r     = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
  auto   x64   = static_cast<fp64_t>(r) * fp64_t::FP_MAX - fp64_t::FP_MAX;
  auto   x128  = static_cast<fp128_t>(x64);
  auto   x64_2 = static_cast<fp64_t>(x128);
  EXPECT_EQ(x64, x64_2);
}

TEST(FixedPointTest, Conversion_64_64)
{
  // Get raw value
  fp128_t one(1);
  fp128_t zero_point_five(0.5);
  fp128_t one_point_five(1.5);
  fp128_t two_point_five(2.5);
  fp128_t m_one_point_five(-1.5);

  EXPECT_EQ(zero_point_five.Data(), static_cast<int128_t>(0x8000000000000000));
  EXPECT_EQ(one.Data(), static_cast<int128_t>(1) << 64);
  EXPECT_EQ(one_point_five.Data(), static_cast<int128_t>(0x18) << 60);
  EXPECT_EQ(two_point_five.Data(), static_cast<int128_t>(0x28) << 60);

  // Convert from raw value
  fp128_t two_point_five_raw(2, 0x8000000000000000);
  fp128_t m_two_point_five_raw(-2, 0x8000000000000000);
  EXPECT_EQ(two_point_five, two_point_five_raw);
  EXPECT_EQ(m_one_point_five, m_two_point_five_raw);

  // Extreme cases:
  // smallest possible double representable to a FixedPoint
  fp128_t infinitesimal(0.00000000000000000009);
  // Largest double fractional closest to one, representable to a FixedPoint
  fp128_t almost_one(0.999999999999999944);
  // Largest fractional closest to one, representable to a FixedPoint
  fp128_t largest_int(std::numeric_limits<int64_t>::max() - 1, 0UL);  // NOLINT

  // Smallest possible integer, increase by one, in order to allow for the fractional part.
  fp128_t smallest_int(std::numeric_limits<int64_t>::min() + 2, 0UL);  // NOLINT

  // Largest possible Fixed Point number.
  fp128_t largest_fixed_point = largest_int + fp128_t(0, fp128_t::LARGEST_FRACTION);  // almost_one;

  // Smallest possible Fixed Point number.
  fp128_t smallest_fixed_point =
      smallest_int - fp128_t(0, fp128_t::LARGEST_FRACTION);  // almost_one;

  EXPECT_EQ(infinitesimal.Data(), fp128_t::SMALLEST_FRACTION);
  // Commented out, double does not give adequate precision to represent the largest fractional part
  // representable by fp128
  // EXPECT_EQ(almost_one.Data(), fp128_t::LARGEST_FRACTION);
  EXPECT_EQ(largest_int.Data(), fp128_t::MAX_INT);
  EXPECT_EQ(smallest_int.Data(), fp128_t::MIN_INT);
  EXPECT_EQ(largest_fixed_point.Data(), fp128_t::MAX);
  EXPECT_EQ(smallest_fixed_point.Data(), fp128_t::MIN);
  EXPECT_EQ(fp128_t::MIN, (static_cast<uint128_t>(0x8000000000000001) << 64) | 0x0000000000000001);
  EXPECT_EQ(fp128_t::MAX, (static_cast<uint128_t>(0x7ffffffffffffffe) << 64) | 0xffffffffffffffff);

  // We cannot be smaller than the actual negative integer of the actual type
  EXPECT_TRUE(smallest_fixed_point.Data() > std::numeric_limits<int128_t>::min());
  // On the other hand we expect to be exactly the same as the largest positive integer of
  EXPECT_TRUE(largest_fixed_point.Data() < std::numeric_limits<int128_t>::max());

  EXPECT_EQ(fp128_t::TOLERANCE.Data(), 0x100000000000);
  EXPECT_EQ(fp128_t::DECIMAL_DIGITS, 18);

  double r    = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
  auto   x128 = static_cast<fp128_t>(r) * static_cast<fp128_t>(fp64_t::FP_MAX) -
              static_cast<fp128_t>(fp64_t::FP_MAX);
  auto x64    = static_cast<fp64_t>(x128);
  auto x128_2 = static_cast<fp128_t>(x64);
  EXPECT_NEAR(static_cast<double>(x128), static_cast<double>(x128_2),
              static_cast<double>(fp64_t::TOLERANCE));

  x128 = static_cast<fp128_t>(r) * static_cast<fp128_t>(fp32_t::FP_MAX) -
         static_cast<fp128_t>(fp32_t::FP_MAX);
  auto x32 = static_cast<fp32_t>(x128);
  x128_2   = static_cast<fp128_t>(x32);
  EXPECT_NEAR(static_cast<double>(x128), static_cast<double>(x128_2),
              static_cast<double>(fp32_t::TOLERANCE));
}

TEST(FixedPointTest, Constants_16_16)
{
  EXPECT_TRUE(fp32_t::CONST_E.Near(2.718281828459045235360287471352662498));
  EXPECT_TRUE(fp32_t::CONST_E == fp32_t(2, 0xB7E1));  // 2.718281828459045235360287471352662498
  EXPECT_TRUE(fp32_t::CONST_LOG2E.Near(1.442695040888963407359924681001892137));
  EXPECT_TRUE(fp32_t::CONST_LOG2E == fp32_t(1, 0x7154));  // 1.442695040888963407359924681001892137
  EXPECT_TRUE(fp32_t::CONST_LOG210.Near(3.32192809488736234787));
  EXPECT_TRUE(fp32_t::CONST_LOG210 == fp32_t(3, 0x5269));  // 3.32192809488736234787
  EXPECT_TRUE(fp32_t::CONST_LOG10E.Near(0.434294481903251827651128918916605082));
  EXPECT_TRUE(fp32_t::CONST_LOG10E == fp32_t(0, 0x6F2D));  // 0.434294481903251827651128918916605082
  EXPECT_TRUE(fp32_t::CONST_LN2.Near(0.693147180559945309417232121458176568));
  EXPECT_TRUE(fp32_t::CONST_LN2 == fp32_t(0, 0xB172));  // 0.693147180559945309417232121458176568
  EXPECT_TRUE(fp32_t::CONST_LN10.Near(2.302585092994045684017991454684364208));
  EXPECT_TRUE(fp32_t::CONST_LN10 == fp32_t(2, 0x4D76));  // 2.302585092994045684017991454684364208
  EXPECT_TRUE(fp32_t::CONST_PI.Near(3.141592653589793238462643383279502884));
  EXPECT_TRUE(fp32_t::CONST_PI == fp32_t(3, 0x243F));  // 3.141592653589793238462643383279502884
  EXPECT_TRUE(fp32_t::CONST_PI_2.Near(1.570796326794896619231321691639751442));
  EXPECT_TRUE(fp32_t::CONST_PI_2 == fp32_t(1, 0x921F));  // 1.570796326794896619231321691639751442
  EXPECT_TRUE(fp32_t::CONST_PI_4.Near(0.785398163397448309615660845819875721));
  EXPECT_TRUE(fp32_t::CONST_PI_4 == fp32_t(0, 0xC90F));  // 0.785398163397448309615660845819875721
  EXPECT_TRUE(fp32_t::CONST_INV_PI.Near(0.318309886183790671537767526745028724));
  EXPECT_TRUE(fp32_t::CONST_INV_PI == fp32_t(0, 0x517C));  // 0.318309886183790671537767526745028724
  EXPECT_TRUE(fp32_t::CONST_TWO_INV_PI.Near(0.636619772367581343075535053490057448));
  EXPECT_TRUE(fp32_t::CONST_TWO_INV_PI ==
              fp32_t(0, 0xA2F9));  // 0 .636619772367581343075535053490057448
  EXPECT_TRUE(fp32_t::CONST_TWO_INV_SQRTPI.Near(1.128379167095512573896158903121545172));
  EXPECT_TRUE(fp32_t::CONST_TWO_INV_SQRTPI ==
              fp32_t(1, 0x20DD));  // 1.128379167095512573896158903121545172
  EXPECT_TRUE(fp32_t::CONST_SQRT2.Near(1.414213562373095048801688724209698079));
  EXPECT_TRUE(fp32_t::CONST_SQRT2 == fp32_t(1, 0x6A09));  // 1.414213562373095048801688724209698079
  EXPECT_TRUE(fp32_t::CONST_INV_SQRT2.Near(0.707106781186547524400844362104849039));
  EXPECT_TRUE(fp32_t::CONST_INV_SQRT2 ==
              fp32_t(0, 0xB504));  // 0.707106781186547524400844362104849039

  EXPECT_EQ(fp32_t::MAX_INT, 0x7ffe0000);
  EXPECT_EQ(fp32_t::MIN_INT, 0x80020000);
  EXPECT_EQ(fp32_t::MAX, 0x7ffeffff);
  EXPECT_EQ(fp32_t::MIN, 0x80010001);
  EXPECT_EQ(fp32_t::MAX_EXP.Data(), 0x000a65adL);
  EXPECT_EQ(static_cast<int32_t>(fp32_t::MIN_EXP.Data()), static_cast<int32_t>(0xfff59a53L));
}

TEST(FixedPointTest, Constants_32_32)
{
  EXPECT_TRUE(fp64_t::CONST_E.Near(2.718281828459045235360287471352662498));
  EXPECT_TRUE(fp64_t::CONST_E == fp64_t(2, 0xB7E15162));  // 2.718281828459045235360287471352662498
  EXPECT_TRUE(fp64_t::CONST_LOG2E.Near(1.442695040888963407359924681001892137));
  EXPECT_TRUE(fp64_t::CONST_LOG2E ==
              fp64_t(1, 0x71547652));  // 1.442695040888963407359924681001892137
  EXPECT_TRUE(fp64_t::CONST_LOG210.Near(3.32192809488736234787));
  EXPECT_TRUE(fp64_t::CONST_LOG210 == fp64_t(3, 0x5269E12F));  // 3.32192809488736234787
  EXPECT_TRUE(fp64_t::CONST_LOG10E.Near(0.434294481903251827651128918916605082));
  EXPECT_TRUE(fp64_t::CONST_LOG10E ==
              fp64_t(0, 0x6F2DEC54));  // 0.434294481903251827651128918916605082
  EXPECT_TRUE(fp64_t::CONST_LN2.Near(0.693147180559945309417232121458176568));
  EXPECT_TRUE(fp64_t::CONST_LN2 ==
              fp64_t(0, 0xB17217F7));  // 0.693147180559945309417232121458176568
  EXPECT_TRUE(fp64_t::CONST_LN10.Near(2.302585092994045684017991454684364208));
  EXPECT_TRUE(fp64_t::CONST_LN10 ==
              fp64_t(2, 0x4D763776));  // 2.302585092994045684017991454684364208
  EXPECT_TRUE(fp64_t::CONST_PI.Near(3.141592653589793238462643383279502884));
  EXPECT_TRUE(fp64_t::CONST_PI == fp64_t(3, 0x243F6A88));  // 3.141592653589793238462643383279502884
  EXPECT_TRUE(fp64_t::CONST_PI_2.Near(1.570796326794896619231321691639751442));
  EXPECT_TRUE(fp64_t::CONST_PI_2 ==
              fp64_t(1, 0x921FB544));  // 1.570796326794896619231321691639751442
  EXPECT_TRUE(fp64_t::CONST_PI_4.Near(0.785398163397448309615660845819875721));
  EXPECT_TRUE(fp64_t::CONST_PI_4 ==
              fp64_t(0, 0xC90FDAA2));  // 0.785398163397448309615660845819875721
  EXPECT_TRUE(fp64_t::CONST_INV_PI.Near(0.318309886183790671537767526745028724));
  EXPECT_TRUE(fp64_t::CONST_INV_PI ==
              fp64_t(0, 0x517CC1B7));  // 0.318309886183790671537767526745028724
  EXPECT_TRUE(fp64_t::CONST_TWO_INV_PI.Near(0.636619772367581343075535053490057448));
  EXPECT_TRUE(fp64_t::CONST_TWO_INV_PI ==
              fp64_t(0, 0xA2F9836E));  // 0 .636619772367581343075535053490057448
  EXPECT_TRUE(fp64_t::CONST_TWO_INV_SQRTPI.Near(1.128379167095512573896158903121545172));
  EXPECT_TRUE(fp64_t::CONST_TWO_INV_SQRTPI ==
              fp64_t(1, 0x20DD7504));  // 1.128379167095512573896158903121545172
  EXPECT_TRUE(fp64_t::CONST_SQRT2.Near(1.414213562373095048801688724209698079));
  EXPECT_TRUE(fp64_t::CONST_SQRT2 ==
              fp64_t(1, 0x6A09E667));  // 1.414213562373095048801688724209698079
  EXPECT_TRUE(fp64_t::CONST_INV_SQRT2.Near(0.707106781186547524400844362104849039));
  EXPECT_TRUE(fp64_t::CONST_INV_SQRT2 ==
              fp64_t(0, 0xB504F333));  // 0.707106781186547524400844362104849039

  EXPECT_EQ(fp64_t::MAX_INT, 0x7ffffffe00000000);
  EXPECT_EQ(fp64_t::MIN_INT, 0x8000000200000000);
  EXPECT_EQ(fp64_t::MAX, 0x7ffffffeffffffff);
  EXPECT_EQ(fp64_t::MIN, 0x8000000100000001);
  EXPECT_EQ(fp64_t::MAX_EXP.Data(), 0x000000157cd0e6e8LL);
  EXPECT_EQ(fp64_t::MIN_EXP.Data(), 0xffffffea832f1918LL);
}

TEST(FixedPointTest, Constants_64_64)
{
  EXPECT_TRUE(fp128_t::CONST_E.Near(2.718281828459045235360287471352662498));
  EXPECT_TRUE(fp128_t::CONST_E ==
              fp128_t(2, 0xB7E151628AED2A6A));  // 2.718281828459045235360287471352662498
  EXPECT_TRUE(fp128_t::CONST_LOG2E.Near(1.442695040888963407359924681001892137));
  EXPECT_TRUE(fp128_t::CONST_LOG2E ==
              fp128_t(1, 0x71547652B82FE177));  // 1.442695040888963407359924681001892137
  EXPECT_TRUE(fp128_t::CONST_LOG210.Near(3.32192809488736234787));
  EXPECT_TRUE(fp128_t::CONST_LOG210 == fp128_t(3, 0x5269E12F346E2BF9));  // 3.32192809488736234787
  EXPECT_TRUE(fp128_t::CONST_LOG10E.Near(0.434294481903251827651128918916605082));
  EXPECT_TRUE(fp128_t::CONST_LOG10E ==
              fp128_t(0, 0x6F2DEC549B9438CA));  // 0.434294481903251827651128918916605082
  EXPECT_TRUE(fp128_t::CONST_LN2.Near(0.693147180559945309417232121458176568));
  EXPECT_TRUE(fp128_t::CONST_LN2 ==
              fp128_t(0, 0xB17217F7D1CF79AB));  // 0.693147180559945309417232121458176568
  EXPECT_TRUE(fp128_t::CONST_LN10.Near(2.302585092994045684017991454684364208));
  EXPECT_TRUE(fp128_t::CONST_LN10 ==
              fp128_t(2, 0x4D763776AAA2B05B));  // 2.302585092994045684017991454684364208
  EXPECT_TRUE(fp128_t::CONST_PI.Near(3.141592653589793238462643383279502884));
  EXPECT_TRUE(fp128_t::CONST_PI ==
              fp128_t(3, 0x243F6A8885A308D3));  // 3.141592653589793238462643383279502884
  EXPECT_TRUE(fp128_t::CONST_PI_2.Near(1.570796326794896619231321691639751442));
  EXPECT_TRUE(fp128_t::CONST_PI_2 ==
              fp128_t(1, 0x921FB54442D18469));  // 1.570796326794896619231321691639751442
  EXPECT_TRUE(fp128_t::CONST_PI_4.Near(0.785398163397448309615660845819875721));
  EXPECT_TRUE(fp128_t::CONST_PI_4 ==
              fp128_t(0, 0xC90FDAA22168C234));  // 0.785398163397448309615660845819875721
  EXPECT_TRUE(fp128_t::CONST_INV_PI.Near(0.318309886183790671537767526745028724));
  EXPECT_TRUE(fp128_t::CONST_INV_PI ==
              fp128_t(0, 0x517CC1B727220A94));  // 0.318309886183790671537767526745028724
  EXPECT_TRUE(fp128_t::CONST_TWO_INV_PI.Near(0.636619772367581343075535053490057448));
  EXPECT_TRUE(fp128_t::CONST_TWO_INV_PI ==
              fp128_t(0, 0xA2F9836E4E441529));  // 0 .636619772367581343075535053490057448
  EXPECT_TRUE(fp128_t::CONST_TWO_INV_SQRTPI.Near(1.128379167095512573896158903121545172));
  EXPECT_TRUE(fp128_t::CONST_TWO_INV_SQRTPI ==
              fp128_t(1, 0x20DD750429B6D11A));  // 1.128379167095512573896158903121545172
  EXPECT_TRUE(fp128_t::CONST_SQRT2.Near(1.414213562373095048801688724209698079));
  EXPECT_TRUE(fp128_t::CONST_SQRT2 ==
              fp128_t(1, 0x6A09E667F3BCC908));  // 1.414213562373095048801688724209698079
  EXPECT_TRUE(fp128_t::CONST_INV_SQRT2.Near(0.707106781186547524400844362104849039));
  EXPECT_TRUE(fp128_t::CONST_INV_SQRT2 ==
              fp128_t(0, 0xB504F333F9DE6484));  // 0.707106781186547524400844362104849039

  EXPECT_EQ(fp128_t::MAX_INT, (static_cast<uint128_t>(0x7ffffffffffffffe) << 64));
  EXPECT_EQ(fp128_t::MIN_INT, (static_cast<uint128_t>(0x8000000000000002) << 64));
  EXPECT_EQ(fp128_t::MAX, (static_cast<uint128_t>(0x7ffffffffffffffe) << 64) | 0xffffffffffffffff);
  EXPECT_EQ(fp128_t::MIN, (static_cast<uint128_t>(0x8000000000000001) << 64) | 0x0000000000000001);

  EXPECT_EQ(fp128_t::MAX_EXP.Data(), (static_cast<uint128_t>(0x2b) << 64) | 0xab13e5fca20e0000);
  EXPECT_EQ(fp128_t::MIN_EXP.Data(),
            (static_cast<uint128_t>(0xffffffffffffffd4) << 64) | 0x54ec1a035df20000);
}

template <typename T>
class ConversionTest : public ::testing::Test
{
};
TYPED_TEST_CASE(ConversionTest, FixedPointTypes);
TYPED_TEST(ConversionTest, Conversion)
{
  // Positive
  TypeParam one(1);
  TypeParam two(2);

  EXPECT_EQ(static_cast<int>(one), 1);
  EXPECT_EQ(static_cast<int>(two), 2);
  EXPECT_EQ(static_cast<float>(one), 1.0f);
  EXPECT_EQ(static_cast<float>(two), 2.0f);
  EXPECT_EQ(static_cast<double>(one), 1.0);
  EXPECT_EQ(static_cast<double>(two), 2.0);

  // Negative
  TypeParam m_one(-1);
  TypeParam m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one), -1);
  EXPECT_EQ(static_cast<int>(m_two), -2);
  EXPECT_EQ(static_cast<float>(m_one), -1.0f);
  EXPECT_EQ(static_cast<float>(m_two), -2.0f);
  EXPECT_EQ(static_cast<double>(m_one), -1.0);
  EXPECT_EQ(static_cast<double>(m_two), -2.0);

  // _0
  TypeParam zero(0);
  TypeParam m_zero(-0);

  EXPECT_EQ(static_cast<int>(zero), 0);
  EXPECT_EQ(static_cast<int>(m_zero), 0);
  EXPECT_EQ(static_cast<float>(zero), 0.0f);
  EXPECT_EQ(static_cast<float>(m_zero), 0.0f);
  EXPECT_EQ(static_cast<double>(zero), 0.0);
  EXPECT_EQ(static_cast<double>(m_zero), 0.0);
  EXPECT_EQ(static_cast<int>(one), 1);
  EXPECT_EQ(static_cast<unsigned>(one), 1);
  EXPECT_EQ(static_cast<int32_t>(one), 1);
  EXPECT_EQ(static_cast<uint32_t>(one), 1);
  EXPECT_EQ(static_cast<long>(one), 1);
  EXPECT_EQ(static_cast<unsigned long>(one), 1);
  EXPECT_EQ(static_cast<int64_t>(one), 1);
  EXPECT_EQ(static_cast<uint64_t>(one), 1);
}

template <typename T>
class BasicArithmeticTest : public ::testing::Test
{
};
TYPED_TEST_CASE(BasicArithmeticTest, FixedPointTypes);
TYPED_TEST(BasicArithmeticTest, Addition)
{
  // Positive
  TypeParam one(1);
  TypeParam two(2);

  EXPECT_EQ(static_cast<int>(one + two), 3);
  EXPECT_EQ(static_cast<float>(one + two), 3.0f);
  EXPECT_EQ(static_cast<double>(one + two), 3.0);

  // Negative
  TypeParam m_one(-1);
  TypeParam m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one + one), 0);
  EXPECT_EQ(static_cast<int>(m_one + m_two), -3);
  EXPECT_EQ(static_cast<float>(m_one + one), 0.0f);
  EXPECT_EQ(static_cast<float>(m_one + m_two), -3.0f);
  EXPECT_EQ(static_cast<double>(m_one + one), 0.0);
  EXPECT_EQ(static_cast<double>(m_one + m_two), -3.0);

  TypeParam another{one};
  ++another;
  EXPECT_EQ(another, two);

  // _0
  TypeParam zero(0);
  TypeParam m_zero(-0);

  EXPECT_EQ(static_cast<int>(zero), 0);
  EXPECT_EQ(static_cast<int>(m_zero), 0);
  EXPECT_EQ(static_cast<float>(zero), 0.0f);
  EXPECT_EQ(static_cast<float>(m_zero), 0.0f);
  EXPECT_EQ(static_cast<double>(zero), 0.0);
  EXPECT_EQ(static_cast<double>(m_zero), 0.0);

  // Infinitesimal additions
  TypeParam almost_one(0, TypeParam::LARGEST_FRACTION);
  TypeParam infinitesimal(0, TypeParam::SMALLEST_FRACTION);

  // Largest possible fraction and smallest possible fraction should make us the value of 1
  EXPECT_EQ(almost_one + infinitesimal, one);
  // The same for negative
  EXPECT_EQ(-almost_one - infinitesimal, m_one);
}

TYPED_TEST(BasicArithmeticTest, Subtraction)
{
  // Positive
  TypeParam one(1);
  TypeParam two(2);

  EXPECT_EQ(static_cast<int>(two - one), 1);
  EXPECT_EQ(static_cast<float>(two - one), 1.0f);
  EXPECT_EQ(static_cast<double>(two - one), 1.0);

  EXPECT_EQ(static_cast<int>(one - two), -1);
  EXPECT_EQ(static_cast<float>(one - two), -1.0f);
  EXPECT_EQ(static_cast<double>(one - two), -1.0);

  // Negative
  TypeParam m_one(-1);
  TypeParam m_two(-2);

  EXPECT_EQ(static_cast<int>(m_one - one), -2);
  EXPECT_EQ(static_cast<int>(m_one - m_two), 1);
  EXPECT_EQ(static_cast<float>(m_one - one), -2.0f);
  EXPECT_EQ(static_cast<float>(m_one - m_two), 1.0f);
  EXPECT_EQ(static_cast<double>(m_one - one), -2.0);
  EXPECT_EQ(static_cast<double>(m_one - m_two), 1.0);

  // Fractions
  TypeParam almost_three(2, TypeParam::LARGEST_FRACTION);
  TypeParam almost_two(1, TypeParam::LARGEST_FRACTION);

  EXPECT_EQ(almost_three - almost_two, one);
}

TYPED_TEST(BasicArithmeticTest, Multiplication)
{
  // Positive
  TypeParam zero(0);
  TypeParam one(1);
  TypeParam two(2);
  TypeParam three(3);
  TypeParam m_one(-1);

  EXPECT_EQ(two * one, two);
  EXPECT_EQ(one * 2, 2);
  EXPECT_EQ(m_one * zero, zero);
  EXPECT_EQ(m_one * one, m_one);
  EXPECT_EQ(static_cast<float>(two * 2.0f), 4.0f);
  EXPECT_EQ(static_cast<double>(three * 2.0), 6.0);

  EXPECT_EQ(static_cast<int>(one * two), 2);
  EXPECT_EQ(static_cast<float>(one * two), 2.0f);
  EXPECT_EQ(static_cast<double>(one * two), 2.0);

  EXPECT_EQ(static_cast<int>(two * zero), 0);
  EXPECT_EQ(static_cast<float>(two * zero), 0.0f);
  EXPECT_EQ(static_cast<double>(two * zero), 0.0);

  // Extreme cases
  TypeParam almost_one(0, TypeParam::LARGEST_FRACTION);
  TypeParam infinitesimal(0, TypeParam::SMALLEST_FRACTION);
  TypeParam huge(TypeParam::SMALLEST_FRACTION << (TypeParam::FRACTIONAL_BITS - 2), 0);
  TypeParam small(0, TypeParam::SMALLEST_FRACTION << (TypeParam::FRACTIONAL_BITS - 2));

  EXPECT_EQ(almost_one * almost_one, almost_one - infinitesimal);
  EXPECT_EQ(almost_one * infinitesimal, zero);
  EXPECT_EQ(huge * infinitesimal, small);
}

TYPED_TEST(BasicArithmeticTest, Division)
{
  // Positive
  TypeParam zero(0);
  TypeParam one(1);
  TypeParam two(2);

  EXPECT_EQ(static_cast<int>(two / one), 2);
  EXPECT_EQ(static_cast<float>(two / one), 2.0f);
  EXPECT_EQ(static_cast<double>(two / one), 2.0);

  EXPECT_EQ(static_cast<int>(one / two), 0);
  EXPECT_EQ(static_cast<float>(one / two), 0.5f);
  EXPECT_EQ(static_cast<double>(one / two), 0.5);

  // Extreme cases
  TypeParam infinitesimal(0, TypeParam::SMALLEST_FRACTION);
  TypeParam huge(TypeParam::SMALLEST_FRACTION << (TypeParam::FRACTIONAL_BITS - 2), 0);
  TypeParam small(0, TypeParam::SMALLEST_FRACTION << (TypeParam::FRACTIONAL_BITS - 2));

  EXPECT_EQ(small / infinitesimal, huge);
  EXPECT_EQ(infinitesimal / one, infinitesimal);
  EXPECT_EQ(one / huge, infinitesimal * 4);
  EXPECT_EQ(huge / infinitesimal, zero);

  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(two / zero));
  EXPECT_TRUE(TypeParam::IsStateDivisionByZero());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(zero / zero));
  EXPECT_TRUE(TypeParam::IsStateNaN());
}

template <typename T>
class ComparisonTest : public ::testing::Test
{
};
TYPED_TEST_CASE(ComparisonTest, FixedPointTypes);
TYPED_TEST(ComparisonTest, Comparison)
{
  TypeParam zero(0);
  TypeParam one(1);
  TypeParam two(2);

  EXPECT_TRUE(zero < one);
  EXPECT_TRUE(zero < two);
  EXPECT_TRUE(one < two);

  EXPECT_FALSE(zero > one);
  EXPECT_FALSE(zero > two);
  EXPECT_FALSE(one > two);

  EXPECT_FALSE(zero == one);
  EXPECT_FALSE(zero == two);
  EXPECT_FALSE(one == two);

  EXPECT_TRUE(zero == zero);
  EXPECT_TRUE(one == one);
  EXPECT_TRUE(two == two);

  EXPECT_TRUE(zero >= zero);
  EXPECT_TRUE(one >= one);
  EXPECT_TRUE(two >= two);

  EXPECT_TRUE(zero <= zero);
  EXPECT_TRUE(one <= one);
  EXPECT_TRUE(two <= two);

  TypeParam zero_point_five(0.5);
  TypeParam one_point_five(1.5);
  TypeParam two_point_five(2.5);

  EXPECT_TRUE(zero_point_five < one);
  EXPECT_TRUE(zero_point_five < two);
  EXPECT_TRUE(one_point_five < two);

  EXPECT_FALSE(zero_point_five > one);
  EXPECT_FALSE(zero_point_five > two);
  EXPECT_FALSE(one_point_five > two);

  EXPECT_FALSE(zero_point_five == one);
  EXPECT_FALSE(zero_point_five == two);
  EXPECT_FALSE(one_point_five == two);

  EXPECT_TRUE(zero_point_five == zero_point_five);
  EXPECT_TRUE(one_point_five == one_point_five);
  EXPECT_TRUE(two_point_five == two_point_five);

  EXPECT_TRUE(zero_point_five >= zero_point_five);
  EXPECT_TRUE(one_point_five >= one_point_five);
  EXPECT_TRUE(two_point_five >= two_point_five);

  EXPECT_TRUE(zero_point_five <= zero_point_five);
  EXPECT_TRUE(one_point_five <= one_point_five);
  EXPECT_TRUE(two_point_five <= two_point_five);

  TypeParam m_zero(-0);
  TypeParam m_one(-1.0);
  TypeParam m_two(-2);

  EXPECT_TRUE(m_zero > m_one);
  EXPECT_TRUE(m_zero > m_two);
  EXPECT_TRUE(m_one > m_two);

  EXPECT_FALSE(m_zero < m_one);
  EXPECT_FALSE(m_zero < m_two);
  EXPECT_FALSE(m_one < m_two);

  EXPECT_FALSE(m_zero == m_one);
  EXPECT_FALSE(m_zero == m_two);
  EXPECT_FALSE(m_one == m_two);

  EXPECT_TRUE(zero == m_zero);
  EXPECT_TRUE(m_zero == m_zero);
  EXPECT_TRUE(m_one == m_one);
  EXPECT_TRUE(m_two == m_two);

  EXPECT_TRUE(m_zero >= m_zero);
  EXPECT_TRUE(m_one >= m_one);
  EXPECT_TRUE(m_two >= m_two);

  EXPECT_TRUE(m_zero <= m_zero);
  EXPECT_TRUE(m_one <= m_one);
  EXPECT_TRUE(m_two <= m_two);

  EXPECT_TRUE(zero > m_one);
  EXPECT_TRUE(zero > m_two);
  EXPECT_TRUE(one > m_two);

  EXPECT_TRUE(m_two < one);
  EXPECT_TRUE(m_one < two);
}

template <typename T>
class BasicTest : public ::testing::Test
{
};
TYPED_TEST_CASE(BasicTest, FixedPointTypes);
TYPED_TEST(BasicTest, Abs)
{
  TypeParam one(1);
  TypeParam m_one(-1);
  TypeParam one_point_five(1.5);
  TypeParam m_one_point_five(-1.5);
  TypeParam ten(10);
  TypeParam m_ten(-10);
  TypeParam huge(TypeParam::FP_MAX / 2);
  TypeParam e1 = TypeParam::Abs(one);
  TypeParam e2 = TypeParam::Abs(m_one);
  TypeParam e3 = TypeParam::Abs(one_point_five);
  TypeParam e4 = TypeParam::Abs(m_one_point_five);
  TypeParam e5 = TypeParam::Abs(ten);
  TypeParam e6 = TypeParam::Abs(m_ten);
  TypeParam e7 = TypeParam::Abs(-huge);

  EXPECT_EQ(e1, one);
  EXPECT_EQ(e2, one);
  EXPECT_EQ(e3, one_point_five);
  EXPECT_EQ(e4, one_point_five);
  EXPECT_EQ(e5, ten);
  EXPECT_EQ(e6, ten);
  EXPECT_EQ(e7, huge);
}

TYPED_TEST(BasicTest, Remainder)
{
  TypeParam one(1);
  TypeParam m_one(-1);
  TypeParam one_point_five(1.5);
  TypeParam m_one_point_five(-1.5);
  TypeParam ten(10);
  TypeParam m_ten(-10);
  TypeParam x{1.6519711627625};
  TypeParam huge(10000);
  huge >>= 2;
  TypeParam e1 = TypeParam::Remainder(ten, one);
  TypeParam e2 = TypeParam::Remainder(ten, m_one);
  TypeParam e3 = TypeParam::Remainder(ten, one_point_five);
  TypeParam e4 = TypeParam::Remainder(ten, m_one_point_five);
  TypeParam e5 = TypeParam::Remainder(ten, x);
  TypeParam e6 = TypeParam::Remainder(m_ten, x);
  TypeParam e7 = TypeParam::Remainder(huge, x);

  EXPECT_NEAR(static_cast<double>(e1),
              std::remainder(static_cast<double>(ten), static_cast<double>(one)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2),
              std::remainder(static_cast<double>(ten), static_cast<double>(m_one)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3),
              std::remainder(static_cast<double>(ten), static_cast<double>(one_point_five)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e4),
              std::remainder(static_cast<double>(ten), static_cast<double>(m_one_point_five)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e5),
              std::remainder(static_cast<double>(ten), static_cast<double>(x)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e6),
              std::remainder(static_cast<double>(m_ten), static_cast<double>(x)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e7),
              std::remainder(static_cast<double>(huge), static_cast<double>(x)),
              static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(BasicTest, Fmod)
{
  TypeParam one(1);
  TypeParam m_one(-1);
  TypeParam one_point_five(1.5);
  TypeParam m_one_point_five(-1.5);
  TypeParam ten(10);
  TypeParam m_ten(-10);
  TypeParam x{1.6519711627625};
  TypeParam e1 = TypeParam::Fmod(ten, one);
  TypeParam e2 = TypeParam::Fmod(ten, m_one);
  TypeParam e3 = TypeParam::Fmod(ten, one_point_five);
  TypeParam e4 = TypeParam::Fmod(ten, m_one_point_five);
  TypeParam e5 = TypeParam::Fmod(ten, x);
  TypeParam e6 = TypeParam::Fmod(m_ten, x);

  EXPECT_EQ(e1, std::fmod(static_cast<double>(ten), static_cast<double>(one)));
  EXPECT_EQ(e2, std::fmod(static_cast<double>(ten), static_cast<double>(m_one)));
  EXPECT_EQ(e3, std::fmod(static_cast<double>(ten), static_cast<double>(one_point_five)));
  EXPECT_EQ(e4, std::fmod(static_cast<double>(ten), static_cast<double>(m_one_point_five)));
  EXPECT_EQ(e5, std::fmod(static_cast<double>(ten), static_cast<double>(x)));
  EXPECT_EQ(e6, std::fmod(static_cast<double>(m_ten), static_cast<double>(x)));
}

template <typename T>
class TranscendentalTest : public ::testing::Test
{
};
TYPED_TEST_CASE(TranscendentalTest, FixedPointTypes);
TYPED_TEST(TranscendentalTest, Exp)
{
  TypeParam one(1);
  TypeParam two(2);
  TypeParam ten(10);
  TypeParam small(0.0001);
  TypeParam tiny(0, TypeParam::SMALLEST_FRACTION);
  TypeParam negative{-0.40028143};
  TypeParam e1    = TypeParam::Exp(one);
  TypeParam e2    = TypeParam::Exp(two);
  TypeParam e3    = TypeParam::Exp(small);
  TypeParam e4    = TypeParam::Exp(tiny);
  TypeParam e5    = TypeParam::Exp(negative);
  TypeParam e6    = TypeParam::Exp(ten);
  TypeParam e_max = TypeParam::Exp(TypeParam::MAX_EXP);

  EXPECT_NEAR(static_cast<double>(e1) - std::exp(static_cast<double>(one)), 0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2) - std::exp(static_cast<double>(two)), 0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3) - std::exp(static_cast<double>(small)), 0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e4) - std::exp(static_cast<double>(tiny)), 0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e5) - std::exp(static_cast<double>(negative)), 0,
              static_cast<double>(TypeParam::TOLERANCE));

  // For bigger values check relative error
  EXPECT_NEAR((static_cast<double>(e6) - std::exp(static_cast<double>(ten))) /
                  std::exp(static_cast<double>(ten)),
              0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR((static_cast<double>(e_max) - std::exp(static_cast<double>(TypeParam::MAX_EXP))) /
                  std::exp(static_cast<double>(TypeParam::MAX_EXP)),
              0, static_cast<double>(TypeParam::TOLERANCE));

  // Out of range
  TypeParam::StateClear();
  EXPECT_EQ(TypeParam::Exp(TypeParam::MAX_EXP + 1), TypeParam::FP_MAX);
  EXPECT_TRUE(TypeParam::IsStateOverflow());

  // Negative values
  EXPECT_NEAR(static_cast<double>(TypeParam::Exp(-one)) - std::exp(-static_cast<double>(one)), 0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(TypeParam::Exp(-two)) - std::exp(-static_cast<double>(two)), 0,
              static_cast<double>(TypeParam::TOLERANCE));

  // This particular error produces more than 1e-6 error failing the test
  EXPECT_NEAR(static_cast<double>(TypeParam::Exp(-ten)) - std::exp(-static_cast<double>(ten)), 0,
              static_cast<double>(TypeParam::TOLERANCE));
  // The rest pass with fp64_t::TOLERANCE
  EXPECT_NEAR(static_cast<double>(TypeParam::Exp(-small)) - std::exp(-static_cast<double>(small)),
              0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(TypeParam::Exp(-tiny)) - std::exp(-static_cast<double>(tiny)), 0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(TypeParam::Exp(TypeParam::MIN_EXP)) -
                  std::exp(static_cast<double>(fp64_t::MIN_EXP)),
              0, static_cast<double>(TypeParam::TOLERANCE));

  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale;
  scale               = 5.0;
  TypeParam tolerance = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale * 2) - scale;
    TypeParam e      = TypeParam::Exp(x);
    double    e_real = std::exp(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(tolerance * 20));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(tolerance));
}

TYPED_TEST(TranscendentalTest, Pow_positive_x_gt_1)
{
  TypeParam a{1.6519711627625};
  TypeParam two{2};
  TypeParam three{3};
  TypeParam b{1.8464393615723};
  TypeParam e1 = TypeParam::Pow(a, two);
  TypeParam e2 = TypeParam::Pow(a, three);
  TypeParam e3 = TypeParam::Pow(two, b);

  EXPECT_NEAR(static_cast<double>(e1 / std::pow(1.6519711627625, 2)), 1.0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2 / std::pow(1.6519711627625, 3)), 1.0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3 / std::pow(2, 1.8464393615723)), 1.0,
              static_cast<double>(TypeParam::TOLERANCE));

  // Disabled for now, when fp128 will have full accuracy we will compare to the correct value
  // TypeParam base{1.0000018429229975};
  // TypeParam extreme{22741975.0640760175883770};

  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, y, scalex, scaley, margin, tolerance;
  scalex    = 5.0;
  margin    = 1.0;
  tolerance = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r      = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x      = static_cast<TypeParam>(r) * (scalex) + margin;
    r      = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    scaley = std::floor(std::log(static_cast<double>(TypeParam::FP_MAX)) /
                        std::log(static_cast<double>(x)));
    y      = static_cast<TypeParam>(r) * scaley;
    TypeParam::StateClear();
    TypeParam e = TypeParam::Pow(x, y);
    if (TypeParam::IsStateOverflow())
    {
      continue;
    }
    double e_real = std::pow(static_cast<double>(x), static_cast<double>(y));
    delta         = std::abs(static_cast<double>(e) - e_real) / e_real;
    max_error     = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  // Due to accuracy limitations esp in the smaller types, max_error can get quite high
  EXPECT_NEAR(max_error, 0.0, 0.3);
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(tolerance) * 100);
}

TYPED_TEST(TranscendentalTest, Pow_positive_x_lt_1)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, y, scalex, scaley, margin, tolerance;
  scalex    = 1.0;
  margin    = 0.001;
  tolerance = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r      = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x      = static_cast<TypeParam>(r) * (scalex) + margin;
    r      = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    scaley = std::floor(std::log(static_cast<double>(TypeParam::FP_MAX)) /
                        std::log(static_cast<double>(x)));
    y      = static_cast<TypeParam>(r) * scaley;
    TypeParam::StateClear();
    TypeParam e = TypeParam::Pow(x, y);
    if (TypeParam::IsStateOverflow())
    {
      continue;
    }
    double e_real = std::pow(static_cast<double>(x), static_cast<double>(y));
    delta         = std::abs(static_cast<double>(e) - e_real) / e_real;
    max_error     = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  // Due to accuracy limitations esp in the smaller types, max_error can get quite high
  // the cause is the logarithm not being accurate enough in higher values
  EXPECT_NEAR(max_error, 0.0, 1.0);
  EXPECT_NEAR(avg_error, 0.0, 0.001);
}

TYPED_TEST(TranscendentalTest, Pow_negative_x)
{
  TypeParam a{-1.6519711627625};
  TypeParam two{2};
  TypeParam three{3};
  TypeParam e1 = TypeParam::Pow(a, two);
  TypeParam e2 = TypeParam::Pow(a, three);

  EXPECT_NEAR(static_cast<double>(e1 / std::pow(static_cast<double>(a), 2)), 1.0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2 / std::pow(static_cast<double>(a), 3)), 1.0,
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Pow(a, a)));
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, y, scalex, scaley, margin, tolerance;
  scalex    = 10.0;
  margin    = 0.0001;
  tolerance = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r      = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x      = static_cast<TypeParam>(r) * (scalex) + margin;
    r      = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    scaley = std::log(static_cast<double>(TypeParam::FP_MAX)) / std::log(static_cast<double>(x));
    y      = TypeParam::Floor(static_cast<TypeParam>(r - 1) * scaley);
    TypeParam e      = TypeParam::Pow(-x, y);
    double    e_real = std::pow(static_cast<double>(-x), static_cast<double>(y));
    delta            = std::abs(static_cast<double>(e) - e_real);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(tolerance * 20));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(tolerance));
}

TYPED_TEST(TranscendentalTest, Logarithm)
{
  TypeParam one(1);
  TypeParam one_point_five(1.5);
  TypeParam ten(10);
  TypeParam huge(TypeParam::FP_MAX / 2);
  TypeParam small(0.001);
  TypeParam tiny(0, TypeParam::SMALLEST_FRACTION);
  TypeParam e1 = TypeParam::Log2(one);
  TypeParam e2 = TypeParam::Log2(one_point_five);
  TypeParam e3 = TypeParam::Log2(ten);
  TypeParam e4 = TypeParam::Log2(huge);
  TypeParam e5 = TypeParam::Log2(small);
  TypeParam e6 = TypeParam::Log2(tiny);

  EXPECT_NEAR(static_cast<double>(e1), std::log2(static_cast<double>(one)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e2), std::log2(static_cast<double>(one_point_five)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e3), std::log2(static_cast<double>(ten)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e4), std::log2(static_cast<double>(huge)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e5), std::log2(static_cast<double>(small)),
              static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(static_cast<double>(e6), std::log2(static_cast<double>(tiny)),
              static_cast<double>(TypeParam::TOLERANCE));

  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale               = 5.0;
  margin              = 0.0001;
  TypeParam tolerance = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * scale + margin;
    TypeParam l      = TypeParam::Log2(x);
    double    l_real = std::log2(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(l) - l_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(tolerance * 20));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(tolerance));
}

TYPED_TEST(TranscendentalTest, Sqrt)
{
  TypeParam one(1);
  TypeParam one_point_five(1.5);
  TypeParam two(2);
  TypeParam four(4);
  TypeParam ten(10);
  TypeParam huge(10000);
  TypeParam small(0.0001);
  TypeParam tiny(0, TypeParam::SMALLEST_FRACTION);
  TypeParam e1 = TypeParam::Sqrt(one);
  TypeParam e2 = TypeParam::Sqrt(one_point_five);
  TypeParam e3 = TypeParam::Sqrt(two);
  TypeParam e4 = TypeParam::Sqrt(four);
  TypeParam e5 = TypeParam::Sqrt(ten);
  TypeParam e6 = TypeParam::Sqrt(huge);
  TypeParam e7 = TypeParam::Sqrt(small);
  TypeParam e8 = TypeParam::Sqrt(tiny);

  double delta = static_cast<double>(e1) - std::sqrt(static_cast<double>(one));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(one)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e2) - std::sqrt(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e3) - std::sqrt(static_cast<double>(two));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(two)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e3 - TypeParam::CONST_SQRT2);
  EXPECT_NEAR(delta / static_cast<double>(TypeParam::CONST_SQRT2), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e4) - std::sqrt(static_cast<double>(four));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(four)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e5) - std::sqrt(static_cast<double>(ten));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(ten)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e6) - std::sqrt(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(huge)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e7) - std::sqrt(static_cast<double>(small));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(small)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e8) - std::sqrt(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::sqrt(static_cast<double>(tiny)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));

  // Sqrt of a negative
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Sqrt(-one)));

  double    r;
  double    max_error = 0, avg_error = 0;
  TypeParam x, scale;
  scale               = 5.0;
  TypeParam tolerance = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * scale;
    TypeParam s      = TypeParam::Sqrt(x);
    double    s_real = std::sqrt(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(s) - s_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(tolerance));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(tolerance));
}

template <typename T>
class TrigonometryTest : public ::testing::Test
{
};
TYPED_TEST_CASE(TrigonometryTest, FixedPointTypes);
TYPED_TEST(TrigonometryTest, Sin)
{
  TypeParam one(1);
  TypeParam one_point_five(1.5);
  TypeParam huge(2000);
  TypeParam small(0.0001);
  TypeParam tiny(0, TypeParam::SMALLEST_FRACTION);
  TypeParam e1  = TypeParam::Sin(one);
  TypeParam e2  = TypeParam::Sin(one_point_five);
  TypeParam e3  = TypeParam::Sin(TypeParam::_0);
  TypeParam e4  = TypeParam::Sin(huge);
  TypeParam e5  = TypeParam::Sin(small);
  TypeParam e6  = TypeParam::Sin(tiny);
  TypeParam e7  = TypeParam::Sin(TypeParam::CONST_PI);
  TypeParam e8  = TypeParam::Sin(-TypeParam::CONST_PI);
  TypeParam e9  = TypeParam::Sin(TypeParam::CONST_PI * 2);
  TypeParam e10 = TypeParam::Sin(TypeParam::CONST_PI * 4);
  TypeParam e11 = TypeParam::Sin(TypeParam::CONST_PI * 100);
  TypeParam e12 = TypeParam::Sin(TypeParam::CONST_PI_2);
  TypeParam e13 = TypeParam::Sin(-TypeParam::CONST_PI_2);
  TypeParam e14 = TypeParam::Sin(TypeParam::CONST_PI_4);
  TypeParam e15 = TypeParam::Sin(-TypeParam::CONST_PI_4);
  TypeParam e16 = TypeParam::Sin(TypeParam::CONST_PI_4 * 3);

  double delta = static_cast<double>(e1) - std::sin(static_cast<double>(one));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(one)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e2) - std::sin(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e3) - std::sin(static_cast<double>(TypeParam::_0));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e4) - std::sin(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(huge)), 0.0,
              0.002);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e5) - std::sin(static_cast<double>(small));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e6) - std::sin(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(tiny)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e7) - std::sin(static_cast<double>(TypeParam::CONST_PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e8) - std::sin(static_cast<double>((-TypeParam::CONST_PI)));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e9) - std::sin(static_cast<double>((TypeParam::CONST_PI * 2)));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e10) - std::sin(static_cast<double>((TypeParam::CONST_PI * 4)));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e11) - std::sin(static_cast<double>((TypeParam::CONST_PI * 100)));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e12) - std::sin(static_cast<double>((TypeParam::CONST_PI_2)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(TypeParam::CONST_PI_2)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e13) - std::sin(static_cast<double>((-TypeParam::CONST_PI_2)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(-TypeParam::CONST_PI_2)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e14) - std::sin(static_cast<double>((TypeParam::CONST_PI_4)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(TypeParam::CONST_PI_4)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e15) - std::sin(static_cast<double>((-TypeParam::CONST_PI_4)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(-TypeParam::CONST_PI_4)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e16) - std::sin(static_cast<double>((TypeParam::CONST_PI_4 * 3)));
  EXPECT_NEAR(delta / std::sin(static_cast<double>(TypeParam::CONST_PI_4 * 3)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));

  double    r;
  double    max_error = 0, avg_error = 0;
  TypeParam x, scale, margin, tolerance;
  scale     = TypeParam::CONST_PI * 10.0;
  margin    = TypeParam::TOLERANCE;
  tolerance = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale * 2 - margin) - (scale - margin);
    TypeParam e      = TypeParam::Sin(x);
    double    e_real = std::sin(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(TrigonometryTest, Cos)
{
  TypeParam one(1);
  TypeParam one_point_five(1.5);
  TypeParam huge(2000);
  TypeParam small(0.0001);
  TypeParam tiny(0, TypeParam::SMALLEST_FRACTION);
  TypeParam e1  = TypeParam::Cos(one);
  TypeParam e2  = TypeParam::Cos(one_point_five);
  TypeParam e3  = TypeParam::Cos(TypeParam::_0);
  TypeParam e4  = TypeParam::Cos(huge);
  TypeParam e5  = TypeParam::Cos(small);
  TypeParam e6  = TypeParam::Cos(tiny);
  TypeParam e7  = TypeParam::Cos(TypeParam::CONST_PI);
  TypeParam e8  = TypeParam::Cos(-TypeParam::CONST_PI);
  TypeParam e9  = TypeParam::Cos(TypeParam::CONST_PI * 2);
  TypeParam e10 = TypeParam::Cos(TypeParam::CONST_PI * 4);
  TypeParam e11 = TypeParam::Cos(TypeParam::CONST_PI * 100);
  TypeParam e12 = TypeParam::Cos(TypeParam::CONST_PI_2);
  TypeParam e13 = TypeParam::Cos(-TypeParam::CONST_PI_2);
  TypeParam e14 = TypeParam::Cos(TypeParam::CONST_PI_4);
  TypeParam e15 = TypeParam::Cos(-TypeParam::CONST_PI_4);
  TypeParam e16 = TypeParam::Cos(TypeParam::CONST_PI_4 * 3);

  double delta = static_cast<double>(e1) - std::cos(static_cast<double>(one));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(one)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e2) - std::cos(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e3) - std::cos(static_cast<double>(TypeParam::_0));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(TypeParam::_0)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e4) - std::cos(static_cast<double>(huge));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(huge)), 0.0,
              0.012);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e5) - std::cos(static_cast<double>(small));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(small)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e6) - std::cos(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(tiny)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e7) - std::cos(static_cast<double>(TypeParam::CONST_PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e8) - std::cos(static_cast<double>(-TypeParam::CONST_PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e9) - std::cos(static_cast<double>(TypeParam::CONST_PI * 2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e10) - std::cos(static_cast<double>(TypeParam::CONST_PI * 4));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e11) - std::cos(static_cast<double>(TypeParam::CONST_PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e12) - std::cos(static_cast<double>(TypeParam::CONST_PI_2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e13) - std::cos(static_cast<double>(-TypeParam::CONST_PI_2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e14) - std::cos(static_cast<double>(TypeParam::CONST_PI_4));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(TypeParam::CONST_PI_4)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e15) - std::cos(static_cast<double>(-TypeParam::CONST_PI_4));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(-TypeParam::CONST_PI_4)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e16) - std::cos(static_cast<double>(TypeParam::CONST_PI_4 * 3));
  EXPECT_NEAR(delta / std::cos(static_cast<double>(TypeParam::CONST_PI_4 * 3)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));

  double    r;
  double    max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam::CONST_PI * 10.0;
  margin = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale * 2 - margin) - (scale - margin);
    TypeParam e      = TypeParam::Cos(x);
    double    e_real = std::cos(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(TrigonometryTest, Tan)
{
  TypeParam one(1);
  TypeParam one_point_five(1.5);
  TypeParam huge(2000);
  TypeParam small(0.0001);
  TypeParam tiny(0, TypeParam::SMALLEST_FRACTION);
  TypeParam e1  = TypeParam::Tan(one);
  TypeParam e2  = TypeParam::Tan(one_point_five);
  TypeParam e3  = TypeParam::Tan(TypeParam::_0);
  TypeParam e4  = TypeParam::Tan(huge);
  TypeParam e5  = TypeParam::Tan(small);
  TypeParam e6  = TypeParam::Tan(tiny);
  TypeParam e7  = TypeParam::Tan(TypeParam::CONST_PI);
  TypeParam e8  = TypeParam::Tan(-TypeParam::CONST_PI);
  TypeParam e9  = TypeParam::Tan(TypeParam::CONST_PI * 2);
  TypeParam e10 = TypeParam::Tan(TypeParam::CONST_PI * 4);
  TypeParam e11 = TypeParam::Tan(TypeParam::CONST_PI * 100);
  TypeParam e12 = TypeParam::Tan(TypeParam::CONST_PI_4);
  TypeParam e13 = TypeParam::Tan(-TypeParam::CONST_PI_4);
  TypeParam e14 = TypeParam::Tan(TypeParam::CONST_PI_4 * 3);

  double delta = static_cast<double>(e1) - std::tan(static_cast<double>(one));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(one)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e2) - std::tan(static_cast<double>(one_point_five));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(one_point_five)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e3) - std::tan(static_cast<double>(TypeParam::_0));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e4) - std::tan(static_cast<double>(huge));
  // Tan for larger arguments loses precision
  EXPECT_NEAR(delta / std::tan(static_cast<double>(huge)), 0.0, 0.012);
  delta = static_cast<double>(e5) - std::tan(static_cast<double>(small));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e6) - std::tan(static_cast<double>(tiny));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(tiny)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e7) - std::tan(static_cast<double>(TypeParam::CONST_PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e8) - std::tan(static_cast<double>(-TypeParam::CONST_PI));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e9) - std::tan(static_cast<double>(TypeParam::CONST_PI * 2));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e10) - std::tan(static_cast<double>(TypeParam::CONST_PI * 4));
  EXPECT_NEAR(delta, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e11) - std::tan(static_cast<double>(TypeParam::CONST_PI * 100));
  EXPECT_NEAR(delta, 0.0, 0.001);  // Sin for larger arguments loses precision
  delta = static_cast<double>(e12) - std::tan(static_cast<double>(TypeParam::CONST_PI_4));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(TypeParam::CONST_PI_4)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e13) - std::tan(static_cast<double>(-TypeParam::CONST_PI_4));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(-TypeParam::CONST_PI_4)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));
  delta = static_cast<double>(e14) - std::tan(static_cast<double>(TypeParam::CONST_PI_4 * 3));
  EXPECT_NEAR(delta / std::tan(static_cast<double>(TypeParam::CONST_PI_4 * 3)), 0.0,
              static_cast<double>(TypeParam::TOLERANCE));

  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::Tan(TypeParam::CONST_PI_2)));
  EXPECT_TRUE(TypeParam::IsNegInfinity(TypeParam::Tan(-TypeParam::CONST_PI_2)));

  double    r;
  double    max_error = 0, avg_error = 0;
  TypeParam x, scale, margin, tolerance;
  scale     = TypeParam::CONST_PI_2;
  margin    = 0.1;
  tolerance = TypeParam::TOLERANCE;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale * 2 - margin) - (scale - margin);
    TypeParam e      = TypeParam::Tan(x);
    double    e_real = std::tan(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    delta            = e_real != 0.0 ? delta / std::abs(e_real) : delta;
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  // Unfortunately Tan() for fp32_t is not really very accurate close to the edges, which gives a
  // high max_error
  EXPECT_NEAR(max_error, 0.0, 0.2);
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(tolerance));
}

TYPED_TEST(TrigonometryTest, ASin)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam::_1;
  margin = TypeParam::TOLERANCE * 10;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::ASin(x);
    double    e_real = std::asin(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(TrigonometryTest, ACos)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam::_1;
  margin = TypeParam::TOLERANCE * 10;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::ACos(x);
    double    e_real = std::acos(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(TrigonometryTest, ATan)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam{5.0};
  margin = TypeParam::_0;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::ATan(x);
    double    e_real = std::atan(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(TrigonometryTest, ATan2)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, y, scale, margin;
  scale  = TypeParam{2.0};
  margin = TypeParam::_0;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    y                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::ATan2(y, x);
    double    e_real = std::atan2(static_cast<double>(y), static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

template <typename T>
class HyperbolicTest : public ::testing::Test
{
};
TYPED_TEST_CASE(HyperbolicTest, FixedPointTypes);
TYPED_TEST(HyperbolicTest, SinH)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam{5.0};
  margin = TypeParam::_0;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::SinH(x);
    double    e_real = std::sinh(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE * 20));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(HyperbolicTest, CosH)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam{5.0};
  margin = TypeParam::_0;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::CosH(x);
    double    e_real = std::cosh(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE * 20));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(HyperbolicTest, TanH)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam{5.0};
  margin = TypeParam::_0;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::TanH(x);
    double    e_real = std::tanh(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(HyperbolicTest, ASinH)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam{3.0};
  margin = TypeParam::_0;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::ASinH(x);
    double    e_real = std::asinh(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE * 10));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(HyperbolicTest, ACosH)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, offset;
  scale  = TypeParam{2.0};
  offset = TypeParam::_1;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * scale + offset;
    TypeParam e      = TypeParam::ACosH(x);
    double    e_real = std::acosh(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

TYPED_TEST(HyperbolicTest, ATanH)
{
  double    r;
  double    delta, max_error = 0, avg_error = 0;
  TypeParam x, scale, margin;
  scale  = TypeParam{1.0};
  margin = 0.001;
  for (size_t i{0}; i < N; i++)
  {
    r                = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
    x                = static_cast<TypeParam>(r) * (scale - margin) - (scale - margin);
    TypeParam e      = TypeParam::ATanH(x);
    double    e_real = std::atanh(static_cast<double>(x));
    delta            = std::abs(static_cast<double>(e) - e_real);
    max_error        = std::max(max_error, delta);
    avg_error += delta;
  }
  avg_error /= static_cast<double>(N);
  EXPECT_NEAR(max_error, 0.0, static_cast<double>(TypeParam::TOLERANCE * 100));
  EXPECT_NEAR(avg_error, 0.0, static_cast<double>(TypeParam::TOLERANCE));
}

template <typename T>
class NanInfinityTest : public ::testing::Test
{
};
TYPED_TEST_CASE(NanInfinityTest, FixedPointTypes);
TYPED_TEST(NanInfinityTest, nan_inf_tests)
{
  TypeParam m_inf{TypeParam::NEGATIVE_INFINITY};
  TypeParam p_inf{TypeParam::POSITIVE_INFINITY};

  // Basic checks
  EXPECT_TRUE(TypeParam::IsInfinity(m_inf));
  EXPECT_TRUE(TypeParam::IsNegInfinity(m_inf));
  EXPECT_TRUE(TypeParam::IsInfinity(p_inf));
  EXPECT_TRUE(TypeParam::IsPosInfinity(p_inf));
  EXPECT_FALSE(TypeParam::IsNegInfinity(p_inf));
  EXPECT_FALSE(TypeParam::IsPosInfinity(m_inf));

  // Absolute value
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::Abs(m_inf)));
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::Abs(p_inf)));
  EXPECT_EQ(TypeParam::Sign(m_inf), -TypeParam::_1);
  EXPECT_EQ(TypeParam::Sign(p_inf), TypeParam::_1);

  // Comparison checks
  EXPECT_FALSE(m_inf < m_inf);
  EXPECT_TRUE(m_inf <= m_inf);
  EXPECT_TRUE(m_inf < p_inf);
  EXPECT_TRUE(m_inf < TypeParam::_0);
  EXPECT_TRUE(m_inf < TypeParam::FP_MIN);
  EXPECT_TRUE(m_inf < TypeParam::FP_MAX);
  EXPECT_TRUE(m_inf < TypeParam::FP_MAX);
  EXPECT_FALSE(p_inf > p_inf);
  EXPECT_TRUE(p_inf >= p_inf);
  EXPECT_TRUE(p_inf > m_inf);
  EXPECT_TRUE(p_inf > TypeParam::_0);
  EXPECT_TRUE(p_inf > TypeParam::FP_MIN);
  EXPECT_TRUE(p_inf > TypeParam::FP_MAX);

  // Addition checks
  // (-∞) + (-∞) = -∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(m_inf + m_inf));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (+∞) + (+∞) = +∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(p_inf + p_inf));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (-∞) + (+∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(m_inf + p_inf));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // (+∞) + (-∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(p_inf + m_inf));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // Subtraction checks
  // (-∞) - (+∞) = -∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(m_inf - p_inf));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (+∞) - (-∞) = +∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(p_inf - m_inf));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (-∞) - (-∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(m_inf - m_inf));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // (+∞) - (+∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(p_inf - p_inf));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // Multiplication checks
  // (-∞) * (+∞) = -∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(m_inf * p_inf));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (+∞) * (+∞) = +∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(p_inf * p_inf));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // 0 * (+∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::_0 * p_inf));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // 0 * (-∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::_0 * m_inf));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // Division checks
  // 0 / (+∞) = 0
  TypeParam::StateClear();
  EXPECT_EQ(TypeParam::_0 / p_inf, TypeParam::_0);
  // 0 * (-∞) = 0
  EXPECT_EQ(TypeParam::_0 / m_inf, TypeParam::_0);

  // (-∞) / MAX_INT = -∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(m_inf / TypeParam::FP_MAX));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (+∞) / MAX_INT = +∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(p_inf / TypeParam::FP_MAX));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (-∞) / MIN_INT = +∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(m_inf / TypeParam::FP_MIN));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (+∞) / MIN_INT = -∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(p_inf / TypeParam::FP_MIN));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (+∞) / (+∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(p_inf / p_inf));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // (-∞) / (+∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(m_inf / p_inf));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // Exponential checks
  // e ^ (0/0) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Exp(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
}

TYPED_TEST(NanInfinityTest, trans_function_nan_inf_tests)
{
  TypeParam m_inf{TypeParam::NEGATIVE_INFINITY};
  TypeParam p_inf{TypeParam::POSITIVE_INFINITY};

  // e ^ (+∞) = +∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::Exp(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // this is actually normal operation, does not modify the state
  // e ^ (-∞) = 0
  TypeParam::StateClear();
  EXPECT_EQ(TypeParam::Exp(m_inf), TypeParam::_0);

  // x^y checks
  // (-∞) ^ (-∞) = 0
  TypeParam::StateClear();
  EXPECT_EQ(TypeParam::Pow(m_inf, m_inf), TypeParam::_0);

  // (-∞) ^ 0 = 1
  TypeParam::StateClear();
  EXPECT_EQ(TypeParam::Pow(m_inf, TypeParam::_0), TypeParam::_1);

  // (+∞) ^ 0 = 1
  EXPECT_EQ(TypeParam::Pow(p_inf, TypeParam::_0), TypeParam::_1);

  // 0 ^ 0 = 1
  EXPECT_EQ(TypeParam::Pow(TypeParam::_0, TypeParam::_0), TypeParam::_1);

  // 0 ^ (-1) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Pow(TypeParam::_0, -TypeParam::_1)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // (-∞) ^ 1 = -∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(TypeParam::Pow(m_inf, TypeParam::_1)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // (+∞) ^ 1 = +∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::Pow(p_inf, TypeParam::_1)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // x ^ (+∞) = +∞, |x| > 1
  TypeParam x1{1.5};
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::Pow(x1, p_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // x ^ (-∞) = 0, |x| > 1
  EXPECT_EQ(TypeParam::Pow(x1, m_inf), TypeParam::_0);

  // x ^ (+∞) = 0, |x| < 1
  TypeParam x2{0.5};
  EXPECT_EQ(TypeParam::Pow(x2, p_inf), TypeParam::_0);

  // x ^ (-∞) = 0, |x| < 1
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::Pow(x2, m_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // 1 ^ (-∞) = 1
  EXPECT_EQ(TypeParam::Pow(TypeParam::_1, m_inf), TypeParam::_1);

  // 1 ^ (-∞) = 1
  EXPECT_EQ(TypeParam::Pow(TypeParam::_1, p_inf), TypeParam::_1);

  // (-1) ^ (-∞) = 1
  EXPECT_EQ(TypeParam::Pow(-TypeParam::_1, m_inf), TypeParam::_1);

  // (-1) ^ (-∞) = 1
  EXPECT_EQ(TypeParam::Pow(-TypeParam::_1, p_inf), TypeParam::_1);

  // Logarithm checks
  // Log(NaN) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Log(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // Log(-∞) = NaN
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Log(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // Log(+∞) = +∞
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsInfinity(TypeParam::Log(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
}

TYPED_TEST(NanInfinityTest, trig_function_nan_inf_tests)
{
  TypeParam m_inf{TypeParam::NEGATIVE_INFINITY};
  TypeParam p_inf{TypeParam::POSITIVE_INFINITY};

  // Trigonometry checks
  // Sin/Cos/Tan(NaN)
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Sin(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Cos(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Tan(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // Sin/Cos/Tan(+/-∞)
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Sin(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Sin(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Cos(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Cos(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Tan(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::Tan(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // ASin/ACos/ATan/ATan2(NaN)
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ASin(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ACos(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ATan(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ATan2(TypeParam::_0 / TypeParam::_0, TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ATan2(TypeParam::_0, TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // ASin/ACos/ATan(+/-∞)
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ASin(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ASin(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ACos(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ACos(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // ATan2(+/-∞)
  TypeParam::StateClear();
  EXPECT_EQ(TypeParam::ATan(m_inf), -TypeParam::CONST_PI_2);
  EXPECT_EQ(TypeParam::ATan(p_inf), TypeParam::CONST_PI_2);
  EXPECT_EQ(TypeParam::ATan2(TypeParam::_1, m_inf), TypeParam::CONST_PI);
  EXPECT_EQ(TypeParam::ATan2(-TypeParam::_1, m_inf), -TypeParam::CONST_PI);
  EXPECT_EQ(TypeParam::ATan2(TypeParam::_1, p_inf), TypeParam::_0);
  EXPECT_EQ(TypeParam::ATan2(m_inf, m_inf), -TypeParam::CONST_PI_4 * 3);
  EXPECT_EQ(TypeParam::ATan2(p_inf, m_inf), TypeParam::CONST_PI_4 * 3);
  EXPECT_EQ(TypeParam::ATan2(m_inf, p_inf), -TypeParam::CONST_PI_4);
  EXPECT_EQ(TypeParam::ATan2(p_inf, p_inf), TypeParam::CONST_PI_4);
  EXPECT_EQ(TypeParam::ATan2(m_inf, TypeParam::_1), -TypeParam::CONST_PI_2);
  EXPECT_EQ(TypeParam::ATan2(p_inf, TypeParam::_1), TypeParam::CONST_PI_2);

  // SinH/CosH/TanH(NaN)
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::SinH(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::CosH(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::TanH(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // SinH/CosH/TanH(+/-∞)
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(TypeParam::SinH(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::SinH(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::CosH(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::CosH(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(TypeParam::TanH(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::TanH(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());

  // ASinH/ACosH/ATanH(NaN)
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ASinH(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ACosH(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ATanH(TypeParam::_0 / TypeParam::_0)));
  EXPECT_TRUE(TypeParam::IsStateNaN());

  // SinH/CosH/TanH(+/-∞)
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNegInfinity(TypeParam::ASinH(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::ASinH(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ACosH(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsPosInfinity(TypeParam::ACosH(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateInfinity());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ATanH(m_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
  TypeParam::StateClear();
  EXPECT_TRUE(TypeParam::IsNaN(TypeParam::ATanH(p_inf)));
  EXPECT_TRUE(TypeParam::IsStateNaN());
}

}  // namespace test
}  // namespace math
}  // namespace fetch
