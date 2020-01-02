//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

namespace {

using namespace fetch;
using namespace fetch::vm;
using fetch::byte_array::ByteArray;
using fetch::vm_modules::math::UInt256Wrapper;

constexpr platform::Endian ENDIANESS_OF_TEST_DATA{platform::Endian::LITTLE};

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

  void TearDown() override
  {
    if (Test::HasFailure())
    {
      std::cout << stdout.str() << std::endl;
    }
  }

  UInt256Wrapper zero{
      dummy_vm_ptr,
      dummy_typeid,
      0,
  };
  UInt256Wrapper _1{dummy_vm_ptr, UInt256Wrapper::UInt256::_1};
  UInt256Wrapper uint64max{dummy_vm_ptr, dummy_typeid, std::numeric_limits<uint64_t>::max()};
  UInt256Wrapper maximum{dummy_vm_ptr, dummy_typeid, raw_32xFF, ENDIANESS_OF_TEST_DATA};

  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(UInt256Tests, uint256_raw_construction)
{
  UInt256Wrapper fromStdUint64{dummy_vm_ptr, dummy_typeid, uint64_t(42)};
  ASSERT_TRUE(SIZE_IN_BYTES == fromStdUint64.size());

  UInt256Wrapper fromByteArray{dummy_vm_ptr, dummy_typeid, raw_32xFF, ENDIANESS_OF_TEST_DATA};
  ASSERT_TRUE(SIZE_IN_BYTES == fromByteArray.size());

  UInt256Wrapper fromAnotherUInt256{dummy_vm_ptr, dummy_typeid, zero.number()};
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
  EXPECT_TRUE(zero.IsLessThanOrEqual(lesser, greater));
  EXPECT_TRUE(zero.IsGreaterThanOrEqual(lesser, lesser));

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

  carriage_inside.InplaceAdd(Ptr<Object>::PtrFromThis(&carriage_inside),
                             Ptr<Object>::PtrFromThis(&_1));

  EXPECT_EQ(carriage_inside.number().ElementAt(0), uint64_t(0));
  EXPECT_EQ(carriage_inside.number().ElementAt(1), uint64_t(1));

  UInt256Wrapper overcarriage(maximum);
  overcarriage.InplaceAdd(Ptr<Object>::PtrFromThis(&overcarriage), Ptr<Object>::PtrFromThis(&_1));

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
      bigger += UInt256(1u64);

      assert(smaller < bigger, "1<2 is false!");
      assert((smaller > bigger) == false, "1>2 is true!");
      assert(smaller <= bigger, "1<=2 is false!");
      assert((smaller >= bigger) == false, "1>=2 is true!");
      assert(smaller != bigger, "1!=2 is false!");
      assert((smaller == bigger) == false, "1==2 is true!");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_shallow_copy)
{
  static constexpr char const *SOURCE = R"(
      function main()
        var a = UInt256(42u64);
        var b = UInt256(0u64);

        a = b;
        assert(a == b, "shallow copy failed!");

        a += UInt256(1u64);

        assert(a == b, "shallow copy failed!");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_deep_copy)
{
  static constexpr char const *SOURCE = R"(
      function main()
        var a = UInt256(42u64);
        var b = UInt256(0u64);
        var _1 = UInt256(1u64);

        a = b.copy();
        assert(a == b, "deep copy failed!");

        b += _1;
        assert(a < b, "a is corrupted by increasing b!");

        a += _1;
        assert(a == b, "b is corrupted by increasing a!");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_trivial_addition)
{
  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(18446744073709551615u64);
        var b = UInt256(18446744073709551615u64);
        assert(a == b, "Initial constants not equal!");

        var zero = UInt256(0u64);
        var result = a + zero;
        assert(result == a, "a+0 != a");

        result = a + b;
        assert(result > a, "a+b <= a");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_trivial_subtraction)
{
  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(18446744073709551615u64);
        var b = UInt256(18446744073709551615u64);
        assert(a == b, "Initial constants not equal!");

        var zero = UInt256(0u64);
        var result = a - zero;
        assert(result == a, "a-0 != a");

        result = a + b;
        result = result - b;
        assert(result == a, "a+b-b != a");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_addition_and_subtraction_together)
{
  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(18446744073709551615u64);
        var b = UInt256(18446744073709551615u64);
        assert(a == b, "Initial constants not equal!");

        var zero = UInt256(0u64);

        var result = b - a + a - b;
        assert(result == zero, "b - a + a - b != 0");

        assert(a + a == b + b, "a + a != b + b");
        assert(a - b == b - a, "a - b != b - a");

        assert(a == b);

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_addition_exact_match_test)
{
  // exact match addition test based on result from following python script
  // x = 18446744073709551
  // y = 14543531527343513
  // z = x + y
  // print(z)

  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(18446744073709551u64);
        var b = UInt256(14543531527343513u64);
        var c = UInt256(32990275601053064u64);
        var result = UInt256(0u64);
        result = a + b;
        assert(result == c, "a+b != c");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_subtraction_exact_match_test)
{
  // exact match subtraction test based on result from following python script
  // x = 1844674407370955161
  // y = 1564837591513245651
  // z = x - y
  // print(z)

  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(18446744073709551615u64);
        var b = UInt256(15648375915132456516u64);
        var c = UInt256(2798368158577095099u64);
        var result = a - b;
        assert(result == c, "a+b != c");
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
          assert(result == a, "+= a failed!");

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

TEST_F(UInt256Tests, uint256_inplace_addition_exact_match_test)
{
  // exact match addition test based on result from following python script
  // x = 123459188422188846
  // y = 841215164823777945
  // z = x + y
  // print(z)

  // exact match addition test based on result from python computation
  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(123459188422188846u64);
        var b = UInt256(841215164823777945u64);
        var c = UInt256(964674353245966791u64);

        a += b;
        assert(a == c, "a += b != c");

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_inplace_subtraction_exact_match_test)
{
  // exact match subtraction test based on result from following python script
  // x = 123459188422188846
  // y = 41215164823777945
  // z = x - y
  // print(z)

  // exact match addition test based on result from python computation
  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(123459188422188846u64);
        var b = UInt256(41215164823777945u64);
        var c = UInt256(82244023598410901u64);

        a -= b;
        assert(a == c, "a -= b != c");

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_trivial_multiplication)
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

         assert(a * b * one == one * b * a, "Multiplication is not commutative!");

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_trivial_division)
{
  static constexpr char const *SRC = R"(
      function main()
         var a = UInt256(18446744073709551615u64);
         var b = UInt256(9000000000000000000u64);

         var two = UInt256(2u64);
         var zero = UInt256(0u64);
         var one  = UInt256(1u64);

         var result = (a * a) / (a * a);
         assert(result == one, "a/a is not 1!");

         result = zero / a;
         assert(result == zero, "Zero divided by smth is not zero!");

         result = a / one;
         assert(result == a, "/1 result is wrong!");

         result = a * UInt256(3u64);
         result = result / a;
         assert(result == UInt256(3u64), "Division if wrong!");

         assert((a / ( a / two)) / two == one, "Division order is wrong!");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_multiplication_exact_match_test)
{
  // exact match multiplication test based on result from following python script
  // x = 146723186
  // y = 134592642
  // z = x + y
  // print(z)

  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(146723186u64);
        var b = UInt256(134592642u64);
        var c = UInt256(19747861246397412u64);
        var result = UInt256(0u64);
        result = a * b;
        assert(result == c, "a+b != c");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_TRUE(toolkit.Run());
}

TEST_F(UInt256Tests, uint256_division_exact_match_test)
{
  // exact match subtraction test based on result from following python script
  // x = 18446744073709551615
  // y = 145435315
  // z = x / y
  // print(z)

  static constexpr char const *SRC = R"(
      function main()
        var a = UInt256(18446744073709551615u64);
        var b = UInt256(145435315u64);
        var c = UInt256(126838134697u64);
        var result = a / b;
        assert(result == c, "a+b != c");
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

TEST_F(UInt256Tests, uint256_type_casts)
{
  static constexpr char const *TEXT = R"(
      function main()
          var test : UInt256 = UInt256(9000000000000000000u64);
          var correct : UInt64 = 9000000000000000000u64;

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

TEST_F(UInt256Tests, uint256_to_string)
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

TEST_F(UInt256Tests, uint256_sha256_assignment)
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
        function main()
            var sha256hasher = SHA256();

            sha256hasher.update("Hello World!");
            var acquired_digest: UInt256 = sha256hasher.final();

            var expected_digest_BigEndian = Buffer(0);
            expected_digest_BigEndian.fromHex("7F83B1657FF1FC53B92DC18148A1D65DFC2D4B1FA3D677284ADDD200126D9069");

            var acquired_digest_buffer = toBuffer(acquired_digest);

            assert(acquired_digest_buffer == expected_digest_BigEndian, "Resulting digest '0x" + acquired_digest_buffer.toHex() + "' does not match expected digest '0x" + expected_digest_BigEndian.toHex() + "'");
        endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  EXPECT_TRUE(toolkit.Run());
}
}  // namespace
