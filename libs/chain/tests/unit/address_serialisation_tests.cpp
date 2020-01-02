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

#include "chain/address.hpp"
#include "core/serializers/main_serializer.hpp"

#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace serializers {

namespace {

using namespace fetch::chain;
using namespace fetch::byte_array;

TEST(LedgerSerializers, address)
{
  std::array<uint8_t, 32> raw_address{};
  for (std::size_t i = 0; i < raw_address.size(); ++i)
  {
    raw_address[i] = static_cast<uint8_t>(i);
  }
  Address a{raw_address};
  Address b;

  MsgPackSerializer stream;
  stream << a;
  stream.seek(0);
  stream >> b;
  EXPECT_EQ(a, b);
}

}  // namespace

}  // namespace serializers
}  // namespace fetch
