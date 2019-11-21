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

#include "gmock/gmock.h"
#include "math/standard_functions/log.hpp"
#include "vm_modules/math/bignumber.hpp"
#include "vm_modules/math/math.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_test_toolkit.hpp"

using namespace fetch::vm;

namespace {

using fetch::byte_array::ByteArray;
using fetch::vm_modules::math::UInt256Wrapper;

const ByteArray raw_32xFF{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

const ByteArray raw_32xAA{
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
};

const ByteArray raw_25xFF{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_24xFF{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_24xAA{
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_17xFF{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_16xFF{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_16xAA{
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_09xFF{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_09xAA{
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_08xFF{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_1234567890{
    0xD2, 0x02, 0x96, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_1234567890123{
    0xcb, 0x04, 0xfb, 0x71, 0x1f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_1234567890123456{
    0xC0, 0xBA, 0x8A, 0x3C, 0xD5, 0x62, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const ByteArray raw_1234567890123456789{
    0x15, 0x81, 0xE9, 0x7D, 0xF4, 0x10, 0x22, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Long integer to double conversion was performed using Wolfram Alfa.
const std::vector<std::pair<ByteArray, double>> TO_DOUBLE_INPUTS{
    {raw_32xFF, 1.15792089237316e+77},
    {raw_32xAA, 7.71947261582108e+76},
    {raw_25xFF, 1.60693804425899e+60},
    {raw_24xFF, 6.27710173538668e+57},
    {raw_24xAA, 4.18473449025779e+57},
    {raw_17xFF, 8.71122859317602e+40},
    {raw_16xFF, 3.40282366920938e+38},
    {raw_16xAA, 2.26854911280626e+38},
    {raw_09xFF, 4.72236648286965e+21},
    {raw_09xAA, 3.14824432191310e+21},
    {raw_08xFF, 1.84467440737096e+19},
    {raw_1234567890, 1234567890.},
    {raw_1234567890123, 1234567890123.},
    {raw_1234567890123456, 1234567890123456.},
    {raw_1234567890123456789, 1234567890123456789.}};

class UInt256Tests : public ::testing::Test
{
public:
  static constexpr auto           dummy_typeid  = fetch::vm::TypeIds::UInt64;
  static constexpr fetch::vm::VM *dummy_vm_ptr  = nullptr;
  static constexpr std::size_t    SIZE_IN_BITS  = 256;
  static constexpr std::size_t    SIZE_IN_BYTES = SIZE_IN_BITS / 8;

  UInt256Wrapper zero{dummy_vm_ptr, dummy_typeid, 0};
  UInt256Wrapper uint64max{dummy_vm_ptr, dummy_typeid, std::numeric_limits<uint64_t>::max()};
  UInt256Wrapper maximum{dummy_vm_ptr, dummy_typeid, raw_32xFF};

  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(UInt256Tests, uint256_raw_construction)
{
  UInt256Wrapper fromStdUint64(dummy_vm_ptr, dummy_typeid, uint64_t(42));
  ASSERT_TRUE(SIZE_IN_BYTES == fromStdUint64.size());

  UInt256Wrapper fromByteArray(dummy_vm_ptr, dummy_typeid, raw_32xFF);
  ASSERT_TRUE(SIZE_IN_BYTES == fromByteArray.size());

  UInt256Wrapper fromAnotherUInt256(dummy_vm_ptr, dummy_typeid, zero.number());
  ASSERT_TRUE(SIZE_IN_BYTES == fromAnotherUInt256.size());
}

TEST_F(UInt256Tests, uint256_raw_comparisons)
{
  Ptr<Object> greater = Ptr<Object>::PtrFromThis(&maximum);
  Ptr<Object> lesser  = Ptr<Object>::PtrFromThis(&zero);

  EXPECT_TRUE(zero.IsEqual(lesser, lesser));
  EXPECT_TRUE(zero.IsNotEqual(lesser, greater));
  EXPECT_TRUE(zero.IsGreaterThan(greater, lesser));
  EXPECT_TRUE(zero.IsLessThan(lesser, greater));

  EXPECT_FALSE(zero.IsEqual(lesser, greater));
  EXPECT_FALSE(zero.IsGreaterThan(lesser, greater));
  EXPECT_FALSE(zero.IsGreaterThan(lesser, lesser));
  EXPECT_FALSE(zero.IsLessThan(lesser, lesser));
  EXPECT_FALSE(zero.IsLessThan(greater, lesser));
}

TEST_F(UInt256Tests, uint256_raw_increase)
{
  // Increase is tested via digit carriage while incrementing.
  UInt256Wrapper carriage_inside(uint64max);

  carriage_inside.Increase();

  EXPECT_EQ(carriage_inside.number().ElementAt(0), uint64_t(0));
  EXPECT_EQ(carriage_inside.number().ElementAt(1), uint64_t(1));

  UInt256Wrapper overcarriage(maximum);

  overcarriage.Increase();

  ASSERT_TRUE(
      zero.IsEqual(Ptr<Object>::PtrFromThis(&zero), Ptr<Object>::PtrFromThis(&overcarriage)));
}

TEST_F(UInt256Tests, uint256_comparisons)
{
  // WARNING! This test passes fine, but the actual values are different from expected ones,
  // because from any integer above 9223372036854775807 the etch VM creates UInt64 with
  // 0x7fffffffffffffff.
  // WARNING! using a value >= 0x7fffffffffffffff (>= 9223372036854775807)
  // in Etch code could fail (other) unit tests, this behavior is still a subject for further
  // investigation.
  // NOTE: Initializing UInt256 with 9223372036854775807 is ok, and after increase (+1) the actual
  // value becomes 9223372036854775808, but it does not corrupt VM state (as initializing UInt64
  // with 9223372036854775808 does).

  static constexpr char const *TEXT = R"(
    function main() : Bool
        var ok : Bool = true;
        var uint64_max = 9223372036854775807u64;

        var smaller = UInt256(uint64_max);
        var bigger = UInt256(uint64_max);
        bigger.increase();

        var gt : Bool = smaller > bigger;
        ok = ok && !gt;

        var ls : Bool = smaller < bigger;
        ok = ok && ls;

        var eq : Bool = smaller == bigger;
        ok = ok && !eq;

        var ne : Bool = smaller != bigger;
        ok = ok && ne;

        return true;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result_is_ok = res.Get<bool>();
  EXPECT_TRUE(result_is_ok);
}

TEST_F(UInt256Tests, uint256_assignment)
{
  static constexpr char const *TEXT = R"(
    function main() : Bool
      var ok : Bool = true;

      var a = UInt256(42u64);
      var b = UInt256(0u64);

      ok = ok && a != b;

      a = b;

      ok = ok && a == b;

      a = SHA256().final();
      // e.g. a == e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

      ok = ok && a != b;

      return ok;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result_is_ok = res.Get<bool>();
  EXPECT_TRUE(result_is_ok);
}

TEST_F(UInt256Tests, uint256_size)
{
  static constexpr char const *TEXT = R"(
      function main() : UInt64
        var uint256 = UInt256(0u64);
        var size = uint256.size();
        return size;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const size = res.Get<uint64_t>();

  EXPECT_TRUE(SIZE_IN_BYTES == size);
}

TEST_F(UInt256Tests, uint256_logValue)
{
  static constexpr double LOGARITHM_TOLERANCE  = 5e-4;
  static constexpr double CONVERSION_TOLERANCE = 0.1;

  for (const auto &input : TO_DOUBLE_INPUTS)
  {
    using namespace std;
    UInt256Wrapper n1(dummy_vm_ptr, dummy_typeid, input.first);

    const auto as_double  = ToDouble(n1.number());
    const auto result     = n1.LogValue();
    const auto exp_double = input.second;
    const auto expected   = std::log(exp_double);

    EXPECT_NEAR(as_double, exp_double, exp_double * CONVERSION_TOLERANCE);
    EXPECT_NEAR(result, expected, expected * LOGARITHM_TOLERANCE);
  }

  static constexpr char const *TEXT = R"(
          function main() : Float64
            var number : UInt256 = UInt256(9000000000000000000u64);
            var logY : Float64 = number.logValue();
            return logY;
          endfunction
        )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<double>();

  double const expected = std::log(9000000000000000000ull);

  EXPECT_NEAR(result, expected, expected * LOGARITHM_TOLERANCE);
}

// Disabled until UInt256 type casting is implemented/fixed.
TEST_F(UInt256Tests, DISABLED_uint256_type_casts)
{
  // WARNING: this test will now fail with UInt256 until some fixes.
  static constexpr char const *TEXT = R"(
      function main() : Bool
          var test : UInt256 = UInt256(9000000000000000000u64);
          var correct : UInt64 = 9000000000000000000u64;
          var ok = true;

          var test_float64 = toFloat64(test);
          var correct_float64 = toFloat64(correct);
          ok = ok && (test_float64 == correct_float64);

          var test_int32 = toInt32(test);
          var correct_int32 = toInt32(correct);
          ok = ok && (test_int32 == correct_int32);

          var test_uint32 = toUInt32(test);
          var correct_uint32 = toUInt32(correct);
          ok = ok && (test_uint32 == correct_uint32);

          var test_int64 = toInt64(test);
          var correct_int64 = toInt64(correct);
          ok = ok && (test_int64 == correct_int64);

          var test_uint64 = toUInt64(test);
          var correct_uint64 = toUInt64(correct);
          ok = ok && (test_uint64 == correct_uint64);

          var test_str : String = toString(test);
          var correct_str : String =
          "0000000000000000000000000000000000000000000000007ce66c50e2840000";
          ok = ok && (test_str == correct_str);
          return ok;
      endfunction
    )";
  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result_is_ok = res.Get<bool>();
  EXPECT_TRUE(result_is_ok);
}

}  // namespace
