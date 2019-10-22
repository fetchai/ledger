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

#include "core/containers/history_buffer.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <algorithm>

namespace {

using Value      = int;
using ValueArray = std::vector<Value>;
using Buffer     = fetch::core::HistoryBuffer<Value, 5>;

class HistoryBufferTests : public ::testing::Test
{
protected:

  Buffer buffer_;
};

ValueArray ToValueList(Buffer const &buffer)
{
  ValueArray values{};
  values.reserve(buffer.size());

  for (auto const &element : buffer)
  {
    values.emplace_back(element);
  }

  return values;
}

template <typename ValueContainer>
bool Contains(ValueContainer const &values, Value const &value)
{
  return std::find(values.begin(), values.end(), value) != values.end();
}

TEST_F(HistoryBufferTests, SimpleAccessorChecks)
{
  ASSERT_EQ(0, buffer_.size());

  buffer_.emplace_back(1);
  ASSERT_EQ(1, buffer_.size());
  ASSERT_EQ(1, buffer_.at(0));
  ASSERT_THROW(buffer_.at(1), std::runtime_error);

  buffer_.emplace_back(2);
  ASSERT_EQ(2, buffer_.size());
  ASSERT_EQ(2, buffer_.at(0));
  ASSERT_EQ(1, buffer_.at(1));
  ASSERT_THROW(buffer_.at(2), std::runtime_error);

  buffer_.emplace_back(3);
  ASSERT_EQ(3, buffer_.size());
  ASSERT_EQ(3, buffer_.at(0));
  ASSERT_EQ(2, buffer_.at(1));
  ASSERT_EQ(1, buffer_.at(2));
  ASSERT_THROW(buffer_.at(3), std::runtime_error);

  buffer_.emplace_back(4);
  ASSERT_EQ(4, buffer_.size());
  ASSERT_EQ(4, buffer_.at(0));
  ASSERT_EQ(3, buffer_.at(1));
  ASSERT_EQ(2, buffer_.at(2));
  ASSERT_EQ(1, buffer_.at(3));
  ASSERT_THROW(buffer_.at(4), std::runtime_error);

  buffer_.emplace_back(5);
  ASSERT_EQ(5, buffer_.size());
  ASSERT_EQ(5, buffer_.at(0));
  ASSERT_EQ(4, buffer_.at(1));
  ASSERT_EQ(3, buffer_.at(2));
  ASSERT_EQ(2, buffer_.at(3));
  ASSERT_EQ(1, buffer_.at(4));
  ASSERT_THROW(buffer_.at(5), std::runtime_error);

  buffer_.emplace_back(6);
  ASSERT_EQ(5, buffer_.size());
  ASSERT_EQ(6, buffer_.at(0));
  ASSERT_EQ(5, buffer_.at(1));
  ASSERT_EQ(4, buffer_.at(2));
  ASSERT_EQ(3, buffer_.at(3));
  ASSERT_EQ(2, buffer_.at(4));
  ASSERT_THROW(buffer_.at(5), std::runtime_error);

  buffer_.emplace_back(7);
  ASSERT_EQ(5, buffer_.size());
  ASSERT_EQ(7, buffer_.at(0));
  ASSERT_EQ(6, buffer_.at(1));
  ASSERT_EQ(5, buffer_.at(2));
  ASSERT_EQ(4, buffer_.at(3));
  ASSERT_EQ(3, buffer_.at(4));
  ASSERT_THROW(buffer_.at(5), std::runtime_error);
}

TEST_F(HistoryBufferTests, IteratorAccessTests)
{
  ASSERT_EQ(0, buffer_.size());
  ASSERT_EQ(ToValueList(buffer_), ValueArray{});

  buffer_.emplace_back(1);
  auto const vals = ToValueList(buffer_);
  ASSERT_EQ(ToValueList(buffer_), ValueArray{1});
}

} // namespace