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

#include "core/serialisers/serializable_exception.hpp"

#include <cstdint>
#include <string>

namespace fetch {
namespace serialisers {
namespace interfaces {

template <typename Driver>
class ArrayInterface
{
public:
  ArrayInterface(Driver &serialiser, uint64_t size)
    : serialiser_{serialiser}
    , size_{size}
  {}

  template <typename T>
  void Append(T const &val)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in array serialization"));
    }

    serialiser_ << val;
  }

  bool AppendUsingFunction(std::function<bool(Driver &)> serialise_function)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in array serialization"));
    }
    return serialise_function(serialiser_);
  }

  Driver &serialiser()
  {
    return serialiser_;
  }

private:
  Driver & serialiser_;
  uint64_t size_;
  uint64_t pos_{0};
};

template <typename Driver>
class ArrayDeserialiser
{
public:
  enum
  {
    CODE_FIXED = TypeCodes::ARRAY_CODE_FIXED,
    CODE16     = TypeCodes::ARRAY_CODE16,
    CODE32     = TypeCodes::ARRAY_CODE32
  };

  explicit ArrayDeserialiser(Driver &serialiser)
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
        throw SerializableException(
            std::string("incorrect size opcode for array size: " + std::to_string(int(opcode)) +
                        " vs " + std::to_string(int(CODE_FIXED))));
      }
      size = static_cast<uint32_t>(opcode & TypeCodes::FIXED_VAL_MASK);
      break;
    }
    size_ = static_cast<uint64_t>(size);
  }

  template <typename V>
  void GetNextValue(V &value)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("tried to deserialise more fields in map than there exists."));
    }
    serialiser_ >> value;
  }

  bool GetNextValueUsingFunction(std::function<bool(Driver &)> serialise_function)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("tried to deserialise more fields in map than there exists."));
    }

    return serialise_function(serialiser_);
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
  Driver & serialiser_;
  uint64_t size_{0};
  uint64_t pos_{0};
};

}  // namespace interfaces
}  // namespace serialisers
}  // namespace fetch
