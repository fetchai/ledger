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
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>

using namespace fetch::byte_array;

namespace fetch {

namespace serializers {



TEST(MsgPacker, floats)
{
  // Setup
  ByteArrayBuffer stream;
  double value;

  value = static_cast<double>(2.34);
  stream = ByteArrayBuffer();
  stream << value;
  std::cout << ToHex(stream.data()) << std::endl;
  EXPECT_EQ(FromHex("cb4002b851eb851eb8"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 2.34);





}

}  // namespace serializers
}  // namespace fetch
