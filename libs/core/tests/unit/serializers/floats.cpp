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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>

using namespace fetch::byte_array;

namespace fetch {
namespace serializers {

namespace {

TEST(MsgPacker, floats)
{
  MsgPackSerializer stream;
  double            value;

  value  = static_cast<double>(2.34);
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cb4002b851eb851eb8"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 2.34);
}

}  // namespace

}  // namespace serializers
}  // namespace fetch
