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
class MapInterface
{
public:
  MapInterface(Driver &serialiser, uint64_t size)
    : serialiser_{serialiser}
    , size_{size}
  {}

  template <typename V>
  void Append(char const *key, V const &val)
  {
    Append(static_cast<std::string>(key), val);
  }

  template <typename K, typename V>
  void Append(K key, V const &val)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in map serialization"));
    }
    serialiser_ << key;
    serialiser_ << val;
  }

  bool AppendUsingFunction(std::function<bool(Driver &)> key_serialise,
                           std::function<bool(Driver &)> value_serialise)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in map serialization"));
    }

    if (!key_serialise(serialiser_))
    {
      return false;
    }

    return value_serialise(serialiser_);
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
class MapDeserialiser
{
public:
  enum
  {
    CODE_FIXED = TypeCodes::MAP_CODE_FIXED,
    CODE16     = TypeCodes::MAP_CODE16,
    CODE32     = TypeCodes::MAP_CODE32
  };
  explicit MapDeserialiser(Driver &serialiser)
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
    size_ = static_cast<uint64_t>(size);
  }

  template <typename K, typename V>
  void GetNextKeyPair(K &key, V &value)
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
    serialiser_ >> key >> value;
  }

  bool GetNextKeyPairUsingFunction(std::function<bool(Driver &)> key_deserialise,
                                   std::function<bool(Driver &)> value_deserialise)
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

    if (!key_deserialise(serialiser_))
    {
      return false;
    }

    return value_deserialise(serialiser_);
  }

  template <typename V>
  bool ExpectKeyGetValue(uint8_t key, V &value)
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
    uint8_t k;
    serialiser_ >> k;
    if (k != key)
    {
      throw SerializableException(
          std::string("Key mismatch while deserialising map: " + std::to_string(pos_ - 1) + " / " +
                      std::to_string(size_)) +
          ", " + std::to_string(k) + " != " + std::to_string(key));
    }
    serialiser_ >> value;
    return true;
  }

  template <typename K, typename V>
  bool ExpectKeyGetValue(K const &key, V &value)
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
    K k;
    serialiser_ >> k;
    if (k != key)
    {
      throw SerializableException(std::string("Key mismatch while deserialising map."));
    }
    serialiser_ >> value;
    return true;
  }

  uint64_t size() const
  {
    return size_;
  }

  template <typename K>
  void GetKey(K &key)
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
    serialiser_ >> key;
    state_ = State::VALUE_NEXT;
  }

  template <typename V>
  void GetValue(V &value)
  {
    if (state_ != State::VALUE_NEXT)
    {
      throw SerializableException(std::string("Next entry is not a value in map."));
    }

    serialiser_ >> value;
    state_ = State::KEY_VALUE_NEXT;
  }

  Driver &serialiser()
  {
    return serialiser_;
  }

private:
  enum class State
  {
    KEY_VALUE_NEXT = 0,
    VALUE_NEXT     = 1
  };

  Driver & serialiser_;
  uint64_t size_{0};
  uint64_t pos_{0};
  State    state_{State::KEY_VALUE_NEXT};
};

}  // namespace interfaces
}  // namespace serialisers
}  // namespace fetch
