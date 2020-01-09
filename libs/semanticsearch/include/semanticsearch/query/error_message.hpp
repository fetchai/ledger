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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "core/commandline/vt100.hpp"
#include "core/serializers/group_definitions.hpp"

#include <vector>

namespace fetch {
namespace semanticsearch {

class ErrorMessage
{
public:
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using Token          = fetch::byte_array::Token;

  enum Type
  {
    WARNING,
    SYNTAX_ERROR,
    RUNTIME_ERROR,
    INTERNAL_ERROR,
    INFO,
    APPEND
  };

  ErrorMessage(ConstByteArray filename, ConstByteArray source, ConstByteArray message, Token token,
               Type type = SYNTAX_ERROR);

  ConstByteArray filename() const;
  ConstByteArray source() const;
  ConstByteArray message() const;
  Token          token() const;
  Type           type() const;
  int            line() const;
  uint64_t       character() const;

private:
  template <typename T, typename D>
  friend struct serializers::MapSerializer;

  ConstByteArray filename_;
  ConstByteArray source_;
  ConstByteArray message_;
  Token          token_;
  Type           type_;
};

}  // namespace semanticsearch

namespace serializers {

template <typename D>
struct MapSerializer<semanticsearch::ErrorMessage, D>
{
public:
  using Type       = semanticsearch::ErrorMessage;
  using DriverType = D;

  static constexpr uint8_t TOKEN = 1;
  static constexpr uint8_t TYPE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &input)
  {
    auto map = map_constructor(3);
    map.Append(TOKEN, input.token_);
    map.Append(TYPE, static_cast<int32_t>(input.type_));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &output)
  {
    int32_t type;
    map.ExpectKeyGetValue(TOKEN, output.token_);
    map.ExpectKeyGetValue(TYPE, type);
    output.type_ = static_cast<semanticsearch::ErrorMessage::Type>(type);
  }
};

}  // namespace serializers
}  // namespace fetch
