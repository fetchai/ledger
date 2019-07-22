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

#include "core/serializers/byte_array.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/byte_array_buffer.hpp"

#include "gtest/gtest.h"

namespace fetch 
{
namespace serializers 
{

template <typename T>
struct A
{
  using ba_type  = byte_array::ByteArray;
  using cba_type = byte_array::ConstByteArray;

  A()          = default;
  A(A const &) = default;
  A(A &&)      = default;

  A &operator=(A const &) = default;
  A &operator=(A &&) = default;

  A(cba_type const &_x, cba_type const &_y)
    : x{_x + " x"}
    , t{_x + " t", _y + " t"}
    , y{_y + " y"}
  {}

  bool operator==(A const &left) const
  {
    return x == left.x && y == left.y && t == left.t;
  }

  bool operator!=(A const &left) const
  {
    return !(*this == left);
  }

  ba_type x{ba_type{T{}.x}.Append(" x")};
  T       t{"Tx", "Ty"};
  ba_type y{ba_type{T{}.y}.Append(" y")};
};

template <>
struct A<void>
{
  using ba_type  = byte_array::ByteArray;
  using cba_type = byte_array::ConstByteArray;

  A()          = default;
  A(A const &) = default;
  A(A &&)      = default;

  A &operator=(A const &) = default;
  A &operator=(A &&) = default;

  A(cba_type const &_x, cba_type const &_y)
    : x{_x}
    , y{_y}
  {}

  bool operator==(A const &left) const
  {
    return x == left.x && y == left.y;
  }

  ba_type x{"X"};
  ba_type y{"Y"};
};

template< typename X, typename D >
struct ArraySerializer< A< X >, D >
{
public:
  using Type = A< X >;
  using DriverType = D;

  template< typename Constructor >
  static void Serialize(Constructor & array_constructor, Type const & a)
  {
    auto array = array_constructor(3);
    array.Append(a.x);
    array.Append(a.y);
    array.Append(a.t);    
  }

  template< typename ArrayDeserializer >
  static void Deserialize(ArrayDeserializer & array, Type & a)
  {
    array.GetNextValue(a.x);
    array.GetNextValue(a.y);
    array.GetNextValue(a.t);
  }  
};


template< typename D >
struct ArraySerializer< A< void >, D >
{
public:
  using Type = A< void >;
  using DriverType = D;

  template< typename Constructor >
  static void Serialize(Constructor & array_constructor, Type const & a)
  {
    auto array = array_constructor(3);
    array.Append(a.x);
    array.Append(a.y);
  }

  template< typename ArrayDeserializer >
  static void Deserialize(ArrayDeserializer & array, Type & a)
  {
    array.GetNextValue(a.x);
    array.GetNextValue(a.y);
  }  
};

class ByteArrayBufferTest : public testing::Test
{
protected:
  using B = A<A<A<void>>>;

  void SetUp()
  {}

  void TearDown()
  {}

  void test_nested_append_serialisation(ByteArrayBuffer &stream)
  {
    B const            b0{"b0x", "b0y"};
    B const            b1{"b1x", "b1y"};
    constexpr uint64_t x = 3;

    auto const orig_stream_offset = stream.tell();

    // Serialising
    stream.Append(b0, x, b1);

    // De-serialising
    B        b0_d;
    B        b1_d;
    uint64_t x_d = 0;
    stream.seek(orig_stream_offset);
    stream >> b0_d >> x_d >> b1_d;

    EXPECT_EQ(b0, b0_d);
    EXPECT_EQ(b1, b1_d);
    EXPECT_EQ(x, x_d);
  }
};

TEST_F(ByteArrayBufferTest, test_seek_position_is_zero_after_stream_construction)
{
  ByteArrayBuffer stream;
  EXPECT_EQ(0, stream.tell());
}

TEST_F(ByteArrayBufferTest, test_basic_allocate_size)
{
  constexpr std::size_t preallocated_amount = 100;

  ByteArrayBuffer stream;
  stream.Allocate(preallocated_amount);

  EXPECT_EQ(preallocated_amount, stream.size());
  EXPECT_EQ(0, stream.tell());

  constexpr std::size_t delta_amount = 10;
  stream.Allocate(delta_amount);

  EXPECT_EQ(preallocated_amount + delta_amount, stream.size());
  EXPECT_EQ(0, stream.tell());
}

TEST_F(ByteArrayBufferTest, test_allocate_with_offset)
{
  constexpr std::size_t offset              = 50;
  constexpr std::size_t preallocated_amount = offset + 50;

  ByteArrayBuffer stream;
  stream.Allocate(preallocated_amount);
  stream.seek(offset);

  EXPECT_EQ(preallocated_amount, stream.size());
  EXPECT_EQ(offset, stream.tell());

  constexpr std::size_t delta_amount = 10;
  stream.Allocate(delta_amount);

  EXPECT_EQ(preallocated_amount + delta_amount, stream.size());
  EXPECT_EQ(offset, stream.tell());
}

TEST_F(ByteArrayBufferTest, verify_correctness_of_copy_and_comparison_behaviour_of_B_type)
{
  B const b0{"b0", "b0"};
  B       b0_copy{b0};

  // Verifying that both variables have the **same** value
  EXPECT_EQ(b0, b0_copy);

  auto const b0_copy_y_orig_value{b0_copy.t.t.y.Copy()};
  // Modifying value of one of variables
  b0_copy.t.t.y.Append("somethig new");
  // Proving that variables have **different** value
  EXPECT_NE(b0, b0_copy);

  // Reverting variable to it's original value
  b0_copy.t.t.y = b0_copy_y_orig_value;
  // Proving that variables have the **same** value after reverting
  EXPECT_EQ(b0, b0_copy);
}

TEST_F(ByteArrayBufferTest, test_basic)
{
  ByteArrayBuffer stream;
  test_nested_append_serialisation(stream);
}

TEST_F(ByteArrayBufferTest, test_stream_with_preexisting_offset)
{
  constexpr std::size_t preallocated_amount = 10;

  ByteArrayBuffer stream;
  stream.Allocate(preallocated_amount);
  stream.seek(preallocated_amount);
  test_nested_append_serialisation(stream);
}

TEST_F(ByteArrayBufferTest, test_stream_relative_resize_with_preexisting_offset)
{
  constexpr std::size_t preallocated_amount = 100;
  ByteArrayBuffer       stream;

  // Production code under test
  stream.Resize(preallocated_amount, ResizeParadigm::RELATIVE);
  stream.seek(preallocated_amount);

  EXPECT_EQ(preallocated_amount, stream.size());
  EXPECT_EQ(preallocated_amount, stream.data().capacity());
  EXPECT_EQ(preallocated_amount, stream.tell());

  constexpr std::size_t delta_size = 10;
  // Production code under test
  stream.Resize(delta_size, ResizeParadigm::RELATIVE);

  EXPECT_EQ(preallocated_amount + delta_size, stream.size());
  EXPECT_EQ(preallocated_amount + delta_size, stream.data().capacity());
  EXPECT_EQ(preallocated_amount, stream.tell());
}

TEST_F(ByteArrayBufferTest, test_that_default_resize_paradigm_is_relative)
{
  constexpr std::size_t delta_size = 10;
  // Setup
  ByteArrayBuffer stream;

  std::size_t expected_size = 0;
  for (uint64_t i = 0; i < 10; ++i)
  {
    // Production code under test
    stream.Resize(delta_size);

    // Expectations
    expected_size += delta_size;
    EXPECT_EQ(expected_size, stream.size());
    EXPECT_EQ(expected_size, stream.data().capacity());
  }
}

TEST_F(ByteArrayBufferTest, test_stream_absolute_resize_with_preexisting_offset)
{
  constexpr std::size_t small_size          = 30;
  constexpr std::size_t offset              = small_size + 20;
  constexpr std::size_t preallocated_amount = offset + 50;

  // Setup
  ByteArrayBuffer stream;
  stream.Resize(preallocated_amount);
  stream.seek(offset);

  // Production code under test
  stream.Resize(small_size, ResizeParadigm::ABSOLUTE);

  // Expectations
  EXPECT_EQ(small_size, stream.size());
  EXPECT_EQ(preallocated_amount, stream.data().capacity());
  EXPECT_EQ(small_size, stream.tell());
}

}  // namespace serializers
}  // namespace fetch
