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
  static constexpr auto           dummy_typeid  = fetch::vm::TypeIds::UInt256;
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

  UInt256Wrapper fromAnotherUInt256(dummy_vm_ptr, zero.number() + zero.number());
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
  static constexpr char const *TEXT = R"(
    function main()
      var uint64_max = 18446744073709551615u64;
      var smaller = UInt256(uint64_max);
      var bigger = UInt256(uint64_max);
      bigger.increase();

      assert(smaller < bigger, "1<2 is false!");
      assert((smaller > bigger) == false, "1>2 is true!");
      assert(smaller != bigger, "1!=2 is false!");
      assert((smaller == bigger) == false, "1==2 is true!");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_assignment)
{
  static constexpr char const *TEXT = R"(
    function main()
      var a = UInt256(42u64);
      var b = UInt256(0u64);

      a = b;
      assert(a == b, "a == b failed!");

      a = SHA256().final();
      // e.g. a == e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

      assert(a != b, "a != b failed!");

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_addition_subtraction)
{
  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(18446744073709551615u64);
        var b = UInt256(18446744073709551615u64);
        assert(a == b, "Initial constants not equal!");

        var zero = UInt256(0u64);

        var result = a - zero;
        assert(result == a, "a-0 != a");

        result = a + zero;
        assert(result == a, "a+0 != a");

        result = a - a;
        assert(result == zero, "a-a != 0");

        result = a + b;
        assert(result > a, "a+b < a");

        result = result - b;
        assert(result == a, "a+b-b != a");

        result = b - a + a - b;
        assert(result == zero, "b - a + a - b != 0");

        assert(a + a == b + b, "a + a != b + b");
        assert(a - b == b - a, "a - b != b - a");

        assert(a == b);

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_inplace_addition_subtraction)
{
  static constexpr char const *SRC = R"(
        function main()
          var a = UInt256(18446744073709551615u64);
          var b = UInt256(18446744073709551615u64);
          var zero = UInt256(0u64);
          assert(a == b, "Initial constants not equal!");

          var result = UInt256(0u64);
          result += a;
          assert(result == b, "+= a failed!");

          result -= b;
          assert(result == zero, "-= b failed!");

          result += a;
          result += b;
          assert(result == a + b, "+=a +=b failed!");

          result -= a;
          result -= b;
          assert(result == zero, "-=a -=b failed!");
        endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_multiplication_division)
{
  static constexpr char const *SRC = R"(
      function main()
         var a = UInt256(18446744073709551615u64);
         var b = UInt256(9000000000000000000u64);

         var two = UInt256(2u64);
         var zero = UInt256(0u64);
         var one  = UInt256(1u64);

         var result = a + zero;
         result = a * zero;
         assert(result == zero, "*0 result is not 0!");

         result = (a * a) / (a * a);
         assert(result == one, "a/a is not 1!");

         result = zero / a;
         assert(result == zero, "Zero divided by smth is not zero!");

         result = a / one;
         assert(result == a, "/1 result is wrong!");

         assert(a * b * one == one * b * a, "Multiplication is not commutative!");

         result = a * UInt256(3u64);
         result = result / a;
         assert(result == UInt256(3u64), "Division if wrong!");

         assert((a / ( a / two)) / two == one, "Division order is wrong!");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_inplace_multiplication_division)
{
  static constexpr char const *SRC = R"(
    function main()
      var a = UInt256(18446744073709551615u64);
      var two = UInt256(2u64);
      var zero = UInt256(0u64);
      var one  = UInt256(1u64);

      var result = a + zero;
      result *= one;
      assert(result == a, "a*1 result is not a!");

      result /= one;
      assert(result == a, "a/1 is not 1!");

      result *= two;
      result /= a;
      assert(result == two, "In-place div and mul are wrong!");

      result *= zero;
      assert(result == zero, "In-place *0 is not 0!");
      result /= a;
      assert(result == zero, "In-place 0/a is not 0");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_division_by_zero)
{
  static constexpr char const *REGULAR = R"(
      function main()
        var a = UInt256(18446744073709551615u64);
        var zero = UInt256(0u64);
        var result = a / zero;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(REGULAR));
  EXPECT_FALSE(toolkit.Run());

  static constexpr char const *INPLACE = R"(
      function main()
        var a = UInt256(18446744073709551615u64);
        var zero = UInt256(0u64);
        var result = a;
        result /= zero;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(INPLACE));
  EXPECT_FALSE(toolkit.Run());
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

    const auto as_double  = n1.ToFloat64();
    const auto result     = n1.LogValue();
    const auto exp_double = input.second;
    const auto expected   = std::log(exp_double);

    EXPECT_NEAR(as_double, exp_double, exp_double * CONVERSION_TOLERANCE);
    EXPECT_NEAR(result, expected, expected * LOGARITHM_TOLERANCE);
  }

  static constexpr char const *TEXT = R"(
          function main() : Float64
            var number : UInt256 = UInt256(18446744073709551615u64);
            var logY : Float64 = number.logValue();
            return logY;
          endfunction
        )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result = res.Get<double>();

  double const expected = std::log(18446744073709551615ull);

  EXPECT_NEAR(result, expected, expected * LOGARITHM_TOLERANCE);
}

TEST_F(UInt256Tests, uint256_type_casts)
{
  static constexpr char const *TEXT = R"(
      function main()
          var test : UInt256 = UInt256(9000000000000000000u64);
          var correct : UInt64 = 9000000000000000000u64;

          var test_float64 = test.toFloat64();
          var correct_float64 = toFloat64(correct);
          assert(test_float64 == correct_float64, "toFloat64(...) failed");

          var test_int32 = toInt32(test);
          var correct_int32 = toInt32(correct);
          assert(test_int32 == correct_int32, "toInt32(...) failed");

          var test_uint32 = toUInt32(test);
          var correct_uint32 = toUInt32(correct);
          assert(test_uint32 == correct_uint32, "toUInt32(...) failed");

          var test_int64 = toInt64(test);
          var correct_int64 = toInt64(correct);
          assert(test_int64 == correct_int64, "toInt64(...) failed");

          var test_uint64 = toUInt64(test);
          var correct_uint64 = toUInt64(correct);
          assert(test_uint64 == correct_uint64, "toUInt64(...) failed");
      endfunction
    )";
  ASSERT_TRUE(toolkit.Compile(TEXT));
  EXPECT_TRUE(toolkit.Run());
}

// Disabled until UInt256 constructor from bytearray fix/rework.
TEST_F(UInt256Tests, DISABLED_uint256_to_string)
{
  static constexpr char const *TEXT = R"(
      function main()
          var test : UInt256 = UInt256(9000000000000000000u64);
          var test_str : String = toString(test);
          var expected_str_in_big_endian_enc : String =
                "0000000000000000000000000000000000000000000000007ce66c50e2840000";
          assert(test_str == expected_str_in_big_endian_enc, "toString(...) failed");
      endfunction
    )";
  ASSERT_TRUE(toolkit.Compile(TEXT));
  EXPECT_TRUE(toolkit.Run());
}

// Disabled until UInt256 constructor from bytearray fix/rework.
TEST_F(UInt256Tests, DISABLED_uint256_sha256_assignment)
{
  // This test uses a SHA256 hash from empty string
  // 0xe3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
  // String representation of UInt256 is big-endian, so expected String is
  // "55b852781b9995a44c939b64e441ae2724b96f99c8f4fb9a141cfc9842c4b0e3"
  // and the ending 8 bytes (as uint64) are
  // 0xa495991b7852b855 == 11859553537011923029.
  // However, the current conversion result is 1449310910991872227, or
  // 0x141cfc9842c4b0e3, which indicated that either SHA256().final() serialization,
  // or UInt256 constructor-from-bytearray is incorrect.
  static constexpr char const *TEXT = R"(
        function main() : Bool
            var test : UInt256 = SHA256().final();
            var asU64 = toUint64(test);
            return asU64 == 11859553537011923029u64;
        endfunction
      )";
  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  auto const result_is_ok = res.Get<bool>();
  EXPECT_TRUE(result_is_ok);
}
}  // namespace
