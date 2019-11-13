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

#include "vm_modules/math/bignumber.hpp"
#include "vm_modules/math/math.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

using namespace fetch::vm;

namespace {

using ::testing::Between;

using DataType = fetch::vm_modules::math::DataType;
using fetch::byte_array::ByteArray;
using fetch::vm_modules::math::UInt256Wrapper;

class UInt256Tests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};

  static constexpr auto           dummy_typeid     = fetch::vm::TypeIds::Unknown;
  static constexpr fetch::vm::VM *dummy_vm_ptr     = nullptr;
  static constexpr std::size_t    data_bytes_total = 256 / 8;
  static const ByteArray          raw_32xff;

  UInt256Wrapper zero{dummy_vm_ptr, dummy_typeid, 0};
  UInt256Wrapper maximum{dummy_vm_ptr, dummy_typeid, raw_32xff};
};

const ByteArray UInt256Tests::raw_32xff{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

TEST_F(UInt256Tests, uint256_construction)
{
  UInt256Wrapper fromStdUint64(dummy_vm_ptr, dummy_typeid, uint64_t(42));
  ASSERT_TRUE(data_bytes_total == fromStdUint64.size());

  UInt256Wrapper fromByteArray(dummy_vm_ptr, dummy_typeid, ByteArray(data_bytes_total));
  ASSERT_TRUE(data_bytes_total == fromByteArray.size());

  UInt256Wrapper fromAnotherUInt256(dummy_vm_ptr, dummy_typeid, zero.number());
  ASSERT_TRUE(data_bytes_total == fromAnotherUInt256.size());
}

TEST_F(UInt256Tests, uint256_comparison)
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

TEST_F(UInt256Tests, uint256_increase)
{
  // Increase is tested via digit carriage while incrementing
  UInt256Wrapper carriage_inside(dummy_vm_ptr, dummy_typeid, std::numeric_limits<uint64_t>::max());

  carriage_inside.Increase();

  EXPECT_EQ(carriage_inside.number().ElementAt(0), uint64_t(0));
  EXPECT_EQ(carriage_inside.number().ElementAt(1), uint64_t(1));

  UInt256Wrapper overcarriage(maximum);

  overcarriage.Increase();

  ASSERT_TRUE(
      zero.IsEqual(Ptr<Object>::PtrFromThis(&zero), Ptr<Object>::PtrFromThis(&overcarriage)));
}

TEST_F(UInt256Tests, uint256_serialization)
{
  // TODO: impl.
}

TEST_F(UInt256Tests, uint256_json)
{
  // TODO: impl.
}

}  // namespace
