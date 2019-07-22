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

#include "core/assert.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "msg_pack_basetypes.hpp"
#include "msg_pack_serializer.hpp"
#include <iostream>

using namespace fetch;
using namespace fetch::serializers;
using namespace fetch::byte_array;

struct Test
{
  int32_t value;
};

template <typename D>
struct MapSerializer<Test, D>
{
public:
  using Type       = Test;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &input)
  {
    auto map = map_constructor(2);
    map.Append(std::string("compact"), true);
    map.Append(std::string("schema"), input.value);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &output)
  {
    std::string key1;
    bool        val1;
    map.GetNextKeyPair(key1, val1);
    std::cout << key1 << " => " << val1 << std::endl;

    std::string key2;
    int32_t     val2;
    map.GetNextKeyPair(key2, val2);
    std::cout << key2 << " => " << val2 << std::endl;
    output.value = val2;
  }
};

int main()
{
  MsgPackByteArrayBuffer buff;
  Test                   test_input;
  std::vector<uint64_t>  numbers{6, 5, 4, 3, 2, 1};

  test_input.value = 12389812;
  buff << int64_t(39) << true << false << std::string("hello world");  // Test() ;
  buff << test_input << numbers;
  std::cout << PartialHex(buff.data()) << std::endl;

  MsgPackByteArrayBuffer buff2(buff.data());
  int64_t                unpack;
  bool                   test1{false}, test2{false};
  std::string            value;
  std::vector<uint64_t>  new_numbers;
  Test                   up_test;
  buff2 >> unpack >> test1 >> test2 >> value;
  buff2 >> up_test >> new_numbers;
  std::cout << unpack << " " << test1 << " " << test2 << " " << value << std::endl;
  for (auto &a : new_numbers)
  {
    std::cout << a << ", ";
  }
  std::cout << std::endl;
  return 0;
}
