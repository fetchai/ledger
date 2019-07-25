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
#include "core/serializers/typed_byte_array_buffer.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>

using namespace fetch::byte_array;

namespace fetch {

namespace serializers {

TEST(MsgPacker, long_strings)
{
  // Setup
  ByteArrayBuffer stream;
  ByteArray       value;
  ByteArray       value2;
  ByteArray       text_buffer;

  text_buffer.Resize((1ull << 16) + 20);
  for (std::size_t j = 0; j < text_buffer.size(); ++j)
  {
    text_buffer[j] = 'a' + static_cast<uint8_t>(j % 26);
  }

  // len(value) = 65526
  value  = text_buffer.SubArray(0, 65526);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafff66162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65527
  value  = text_buffer.SubArray(0, 65527);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafff76162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65528
  value  = text_buffer.SubArray(0, 65528);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafff86162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65529
  value  = text_buffer.SubArray(0, 65529);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafff96162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65530
  value  = text_buffer.SubArray(0, 65530);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafffa6162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65531
  value  = text_buffer.SubArray(0, 65531);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafffb6162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65532
  value  = text_buffer.SubArray(0, 65532);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafffc6162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65533
  value  = text_buffer.SubArray(0, 65533);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafffd6162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65534
  value  = text_buffer.SubArray(0, 65534);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("dafffe6162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65535
  value  = text_buffer.SubArray(0, 65535);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("daffff6162636465666768696a6b6c6d6e6f707172737475767778797a616263"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65536
  value  = text_buffer.SubArray(0, 65536);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100006162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65537
  value  = text_buffer.SubArray(0, 65537);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100016162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65538
  value  = text_buffer.SubArray(0, 65538);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100026162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65539
  value  = text_buffer.SubArray(0, 65539);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100036162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65540
  value  = text_buffer.SubArray(0, 65540);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100046162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65541
  value  = text_buffer.SubArray(0, 65541);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100056162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65542
  value  = text_buffer.SubArray(0, 65542);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100066162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65543
  value  = text_buffer.SubArray(0, 65543);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100076162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65544
  value  = text_buffer.SubArray(0, 65544);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100086162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);

  // len(value) = 65545
  value  = text_buffer.SubArray(0, 65545);
  stream = ByteArrayBuffer();
  stream << value;
  EXPECT_EQ(FromHex("db000100096162636465666768696a6b6c6d6e6f707172737475767778797a61"),
            stream.data().SubArray(0, 32));
  stream.seek(0);
  value2 = "";
  stream >> value2;
  EXPECT_EQ(value, value2);
}

}  // namespace serializers
}  // namespace fetch
