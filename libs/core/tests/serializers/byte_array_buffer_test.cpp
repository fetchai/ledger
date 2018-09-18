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

  bool operator ==(A const &left) const
  {
    return x == left.x && y == left.y && t == left.t;
  }

  bool operator !=(A const &left) const
  {
    return !(*this == left);
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

  bool operator ==(A const &left) const
  {
    return x == left.x && y == left.y;
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


class ByteArrayBufferTest : public testing::Test
{
protected:
  using B = A<A<A<void>>>;

  void SetUp()
  {}

  void TearDown()
  {}

  void test_serialisation_with_stream(ByteArrayBuffer &stream)
  {
    B const b0{"b0x", "b0y"};
    B const b1{"b1x", "b1y"};
    constexpr uint64_t x = 3;

    auto const orig_stream_offset = stream.Tell();

    //* Serialising
    stream.Append(b0, x, b1);

    //* De-serialising
    B b0_d;
    B b1_d;
    uint64_t x_d = 0;
    stream.Seek(orig_stream_offset);
    stream >> b0_d >> x_d >> b1_d;

    EXPECT_EQ(b0, b0_d);
    EXPECT_EQ(b1, b1_d);
    EXPECT_EQ(x, x_d);
  }
};

TEST_F(ByteArrayBufferTest, verify_correctness_of_copy_and_comparison_behaviour_of_B_type)
{
  B const b0{"b0", "b0"};
  B       b0_copy{ b0 };

  //* Verifying that both variables have the **same** value
  EXPECT_EQ(b0, b0_copy);

  auto const b0_copy_y_orig_value{ b0_copy.t.t.y.Copy() };
  //* Modifying value of one of variables
  b0_copy.t.t.y.Append("somethig new");
  //* Proving that variables have **different** value
  EXPECT_NE(b0, b0_copy);

  //* Reverting variable to it's original value
  b0_copy.t.t.y = b0_copy_y_orig_value;
  //* Proving that variables have the **same** value after reverting
  EXPECT_EQ(b0, b0_copy);
}

TEST_F(ByteArrayBufferTest, test_basic)
{
  ByteArrayBuffer stream;
  test_serialisation_with_stream(stream);
}

TEST_F(ByteArrayBufferTest, test_stream_with_preexisting_offeset)
{
  constexpr std::size_t preallocated_ammount = 10;

  ByteArrayBuffer stream;
  stream.Allocate(preallocated_ammount);
  stream.Seek(preallocated_ammount);
  test_serialisation_with_stream(stream);
}

}  // namespace

}  // namespace serializers
}  // namespace fetch
