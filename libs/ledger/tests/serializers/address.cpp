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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/block.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
using namespace fetch::ledger;
using namespace fetch::byte_array;

namespace fetch {

namespace serializers {

TEST(LedgerSerializers, address)
{
  std::array<uint8_t, 32> raw_address;
  for (uint8_t i = 0; i < raw_address.size(); ++i)
  {
    raw_address[i] = i;
  }
  Address a{raw_address};
  Address b;

  ByteArrayBuffer stream;
  stream << a;
  stream.seek(0);
  stream >> b;
  EXPECT_EQ(a, b);
}

}  // namespace serializers
}  // namespace fetch
