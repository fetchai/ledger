#pragma once
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

  template <typename F, typename S>
  void Append(F const first, S const &second)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in map serialization"));
    }
    serializer_ << first;
    serializer_ << second;
  }

  bool AppendUsingFunction(std::function<bool(Driver &)> first_serialize,
                           std::function<bool(Driver &)> second_serialize)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in array serialization"));
    }

    if (!first_serialize(serializer_))
    {
      return false;
    }

    return second_serialize(serializer_);
  }

  Driver &serializer()
  {
    return serializer_;
  }

private:
  Driver & serializer_;
  uint64_t size_;
  uint64_t pos_{0};
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
    size_ = static_cast<uint64_t>(size);
  }

  template <typename F, typename S>
  void GetPair(F &first, S &second)
  {
    if (state_ != State::KEY_VALUE_NEXT)
    {
      throw SerializableException(std::string("Next entry is not a key-value pair."));
    }

    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("tried to deserialise more fields in map than there exists."));
    }
    serializer_ >> first >> second;
  }

  bool GetPairUsingFunction(std::function<bool(Driver &)> first_deserialize,
                            std::function<bool(Driver &)> second_deserialize)
  {
    if (state_ != State::KEY_VALUE_NEXT)
    {
      throw SerializableException(std::string("Next entry is not a key-value pair."));
    }

    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("tried to deserialise more fields in map than there exists."));
    }

    if (!first_deserialize(serializer_))
    {
      return false;
    }

    return second_deserialize(serializer_);
  }

  uint64_t size() const
  {
    return size_;
  }

  template <typename F>
  void GetFirst(F &first)
  {
    if (state_ != State::KEY_VALUE_NEXT)
    {
      throw SerializableException(std::string("Next entry is not a key in map."));
    }
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("tried to deserialise more fields in map than there exists."));
    }
    serializer_ >> first;
    state_ = State::VALUE_NEXT;
  }

  template <typename S>
  void GetSecond(S &second)
  {
    if (state_ != State::VALUE_NEXT)
    {
      throw SerializableException(std::string("Next entry is not a value in map."));
    }

    serializer_ >> second;
    state_ = State::KEY_VALUE_NEXT;
  }

  Driver &serializer()
  {
    return serializer_;
  }

private:
  enum class State
  {
    KEY_VALUE_NEXT = 0,
    VALUE_NEXT     = 1
  };

  Driver & serializer_;
  uint64_t size_{0};
  uint64_t pos_{0};
  State    state_{State::KEY_VALUE_NEXT};
};

}  // namespace interfaces
}  // namespace serializers
}  // namespace fetch
