#pragma once
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/group_definitions.hpp"

namespace fetch {
namespace byte_array {

class Token : public ConstByteArray
{
public:
  Token() = default;

  explicit Token(char const *str)
    : ConstByteArray(str)
  {}

  explicit Token(std::string const &str)
    : ConstByteArray(str.c_str())
  {}

  explicit Token(ConstByteArray const &other)
    : ConstByteArray(other)
  {}
  explicit Token(ConstByteArray &&other)
    : ConstByteArray(other)
  {}

  Token(ConstByteArray const &other, std::size_t start, std::size_t length)
    : ConstByteArray(other, start, length)
  {}

  bool operator==(ConstByteArray const &other) const
  {
    return ConstByteArray::operator==(other);
  }

  bool operator!=(ConstByteArray const &other) const
  {
    return !(*this == other);
  }

  void SetType(int const &t)
  {
    type_ = t;
  }
  void SetLine(int const &l)
  {
    line_ = l;
  }
  void SetChar(std::size_t c)
  {
    char_ = c;
  }

  int type() const
  {
    return type_;
  }

  int line() const
  {
    return line_;
  }

  std::size_t character() const
  {
    return char_;
  }

private:
  template <typename T, typename D>
  friend struct serializers::MapSerializer;

  int32_t  type_ = -1;
  int32_t  line_ = 0;
  uint64_t char_ = 0;
};
}  // namespace byte_array

namespace serializers {

template <typename D>
struct MapSerializer<byte_array::Token, D>
{
public:
  using Type       = byte_array::Token;
  using DriverType = D;

  static constexpr uint8_t TYPE      = 1;
  static constexpr uint8_t LINE      = 2;
  static constexpr uint8_t CHARACTER = 3;
  static constexpr uint8_t VALUE     = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &input)
  {
    auto map = map_constructor(4);

    map.Append(TYPE, input.type_);
    map.Append(LINE, input.line_);
    map.Append(CHARACTER, input.char_);
    map.Append(VALUE, static_cast<byte_array::ConstByteArray>(input));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &output)
  {
    int32_t                    type;
    int32_t                    line;
    uint64_t                   character;
    byte_array::ConstByteArray value;

    map.ExpectKeyGetValue(TYPE, type);
    map.ExpectKeyGetValue(LINE, line);
    map.ExpectKeyGetValue(CHARACTER, character);
    map.ExpectKeyGetValue(VALUE, value);

    output       = Type(value);
    output.char_ = character;
    output.line_ = line;
    output.type_ = type;
  }
};

}  // namespace serializers
}  // namespace fetch
