//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/serializers/typed_byte_array_buffer.hpp"
#include "core/serializers/byte_array.hpp"

#include "gtest/gtest.h"
//#include "gmock/gmock.h"

#include <iostream>

namespace fetch {
namespace serializers {

namespace {

template<typename T>
struct A
{
  using ba_type  = byte_array::ByteArray;
  using cba_type = byte_array::ConstByteArray;

  A() = default;
  A(A const&) = default;
  A(A &&) = default;

  A& operator =(A const&) = default;
  A& operator =(A &&) = default;

  A(cba_type const& _x, cba_type const& _y)
    : x{_x + " x"}
    , t{_x + " t", _y + " t"}
    , y{_y + " y"}
  {
  }

  ba_type x {ba_type{T{}.x}.Append(" x")};
  T t{"Tx", "Ty"};
  ba_type y {ba_type{T{}.y}.Append(" y")};
};

template<>
struct A<void>
{
  using ba_type  = byte_array::ByteArray;
  using cba_type = byte_array::ConstByteArray;

  A() = default;
  A(A const&) = default;
  A(A &&) = default;

  A& operator =(A const&) = default;
  A& operator =(A &&) = default;

  A(cba_type const& _x, cba_type const& _y)
    : x{_x}
    , y{_y}
  {
  }

  ba_type x {"X"};
  ba_type y {"Y"};
};

template <typename T, typename X>
void Serialize(T &serializer, A<X> const &a)
{
  serializer.Append(a.x, a.y, a.t);
}

template <typename T, typename X>
void Deserialize(T &serializer, A<X> &a)
{
  serializer >> a.x >> a.y >> a.t;
}

template <typename T>
void Serialize(T &serializer, A<void> const &a)
{
  serializer.Append(a.x, a.y);
}

template <typename T>
void Deserialize(T &serializer, A<void> &a)
{
  serializer >> a.x >> a.y;
}

using B = A<A<A<void>>>;

class ByteArrayBufferTest : public testing::Test
{
protected:
  void SetUp()
  {}

  void TearDown()
  {}
};

TEST_F(ByteArrayBufferTest, test_basic)
{
  B b0{"b0", "b0"};
  B b1{"b1", "b1"};
  uint64_t x = 3;

  ByteArrayBuffer stream;
  stream.Append(b0, x, b1);

  SizeCounter<void> counter_stream;
  counter_stream << b0 << x << b1;

  EXPECT_EQ(counter_stream.size(), stream.size());

  B b0_d;
  B b1_d;
  uint64_t x_d = 0;
  stream >> b0_d >> x_d >> b1_d;

  std::cout << stream.data() << std::endl;
}

}  // namespace

}  // namespace serializers
}  // namespace fetch
