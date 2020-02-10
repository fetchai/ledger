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

#include "core/serialisers/base_types.hpp"

#include <cstdint>

namespace fetch {
namespace serialisers {
namespace interfaces {

template <typename Driver>
class PairInterface
{
public:
  PairInterface(Driver &serialiser, uint64_t size)
    : serialiser_{serialiser}
    , size_{size}
  {}

  bool AppendFirst(std::function<bool(Driver &)> first_serialise)
  {
    return first_serialise(serialiser_);
  }

  bool AppendSecond(std::function<bool(Driver &)> second_serialise)
  {
    return second_serialise(serialiser_);
  }

  Driver &serialiser()
  {
    return serialiser_;
  }

private:
  Driver &serialiser_;
  // 0 = no element, 1 = first only, 2 = second only, 3 = both
  uint64_t size_{0};
};

template <typename Driver>
class PairDeserialiser
{
public:
  enum
  {
    CODE_FIXED = TypeCodes::PAIR_CODE_FIXED,
    CODE16     = TypeCodes::PAIR_CODE16,
    CODE32     = TypeCodes::PAIR_CODE32
  };
  explicit PairDeserialiser(Driver &serialiser)
    : serialiser_{serialiser}
  {
    uint8_t  opcode;
    uint32_t size;
    serialiser_.ReadByte(opcode);
    switch (opcode)
    {
    case CODE16:
    {
      uint16_t tmp;
      serialiser_.ReadBytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(uint16_t));
      tmp  = platform::FromBigEndian(tmp);
      size = static_cast<uint32_t>(tmp);
      break;
    }
    case CODE32:
      serialiser_.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint32_t));
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

  bool GetFirstUsingFunction(std::function<bool(Driver &)> first_deserialise)
  {
    return first_deserialise(serialiser_);
  }

  bool GetSecondUsingFunction(std::function<bool(Driver &)> second_deserialise)
  {
    return second_deserialise(serialiser_);
  }

  uint64_t size() const
  {
    return size_;
  }

  Driver &serialiser()
  {
    return serialiser_;
  }

private:
  Driver &serialiser_;
  // 0 = no element, 1 = first only, 2 = second only, 3 = both
  uint64_t size_{0};
};

}  // namespace interfaces
}  // namespace serialisers
}  // namespace fetch
