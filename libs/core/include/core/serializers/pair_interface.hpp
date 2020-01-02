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

#include "core/serializers/base_types.hpp"

#include <cstdint>

namespace fetch {
namespace serializers {
namespace interfaces {

template <typename Driver>
class PairInterface
{
public:
  PairInterface(Driver &serializer, uint64_t size)
    : serializer_{serializer}
    , size_{size}
  {}

  bool AppendFirst(std::function<bool(Driver &)> first_serialize)
  {
    return first_serialize(serializer_);
  }

  bool AppendSecond(std::function<bool(Driver &)> second_serialize)
  {
    return second_serialize(serializer_);
  }

  Driver &serializer()
  {
    return serializer_;
  }

private:
  Driver &serializer_;
  // 0 = no element, 1 = first only, 2 = second only, 3 = both
  uint64_t size_{0};
};

template <typename Driver>
class PairDeserializer
{
public:
  enum
  {
    CODE_FIXED = TypeCodes::PAIR_CODE_FIXED,
    CODE16     = TypeCodes::PAIR_CODE16,
    CODE32     = TypeCodes::PAIR_CODE32
  };
  explicit PairDeserializer(Driver &serializer)
    : serializer_{serializer}
  {
    uint8_t  opcode;
    uint32_t size;
    serializer_.ReadByte(opcode);
    switch (opcode)
    {
    case CODE16:
    {
      uint16_t tmp;
      serializer_.ReadBytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(uint16_t));
      tmp  = platform::FromBigEndian(tmp);
      size = static_cast<uint32_t>(tmp);
      break;
    }
    case CODE32:
      serializer_.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint32_t));
      size = platform::FromBigEndian(size);
      break;
    default:
      if ((opcode & TypeCodes::FIXED_MASK1) != CODE_FIXED)
      {
        throw SerializableException(std::string("incorrect size opcode for map size."));
      }
      size = static_cast<uint32_t>(opcode & TypeCodes::FIXED_VAL_MASK);
    }
  }

  bool GetFirstUsingFunction(std::function<bool(Driver &)> first_deserialize)
  {
    return first_deserialize(serializer_);
  }

  bool GetSecondUsingFunction(std::function<bool(Driver &)> second_deserialize)
  {
    return second_deserialize(serializer_);
  }

  uint64_t size() const
  {
    return size_;
  }

  Driver &serializer()
  {
    return serializer_;
  }

private:
  Driver &serializer_;
  // 0 = no element, 1 = first only, 2 = second only, 3 = both
  uint64_t size_{0};
};

}  // namespace interfaces
}  // namespace serializers
}  // namespace fetch
