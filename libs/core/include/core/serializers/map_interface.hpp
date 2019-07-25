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

namespace fetch {
namespace serializers {
namespace interfaces {
class MapInterface
{
public:
  MapInterface(MsgPackByteArrayBuffer &serializer, uint64_t size)
    : serializer_{serializer}
    , size_{std::move(size)}
  {}

  template <typename V>
  void Append(uint8_t key, V const &val)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in map serialization"));
    }
    serializer_ << key;
    serializer_ << val;
  }

  template <typename V>
  void Append(char const *key, V const &val)
  {
    Append(static_cast<std::string>(key), val);
  }

  template <typename K, typename V>
  void Append(K const &key, V const &val)
  {
    ++pos_;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in map serialization"));
    }
    serializer_ << key;
    serializer_ << val;
  }

  MsgPackByteArrayBuffer &serializer()
  {
    return serializer_;
  }

private:
  MsgPackByteArrayBuffer &serializer_;
  uint64_t                size_;
  uint64_t                pos_{0};
};

class MapDeserializer
{
public:
  enum
  {
    CODE_FIXED = TypeCodes::MAP_CODE_FIXED,
    CODE16     = TypeCodes::MAP_CODE16,
    CODE32     = TypeCodes::MAP_CODE32
  };
  MapDeserializer(MsgPackByteArrayBuffer &serializer)
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
    serializer_ >> key >> value;
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
    serializer_ >> k;
    if (k != key)
    {
      throw SerializableException(
          std::string("Key mismatch while deserialising map: " + std::to_string(pos_ - 1) + " / " +
                      std::to_string(size_)) +
          ", " + std::to_string(k) + " != " + std::to_string(key));
    }
    serializer_ >> value;
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
    serializer_ >> k;
    if (k != key)
    {
      throw SerializableException(std::string("Key mismatch while deserialising map."));
    }
    serializer_ >> value;
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
    serializer_ >> key;
    state_ = State::VALUE_NEXT;
  }

  template <typename V>
  void GetValue(V &value)
  {
    if (state_ != State::VALUE_NEXT)
    {
      throw SerializableException(std::string("Next entry is not a value in map."));
    }

    serializer_ >> value;
    state_ = State::KEY_VALUE_NEXT;
  }

  MsgPackByteArrayBuffer &serializer()
  {
    return serializer_;
  }

private:
  enum class State
  {
    KEY_VALUE_NEXT = 0,
    VALUE_NEXT     = 1
  };

  MsgPackByteArrayBuffer &serializer_;
  uint64_t                size_{0};
  uint64_t                pos_{0};
  State                   state_{State::KEY_VALUE_NEXT};
};

}  // namespace interfaces
}  // namespace serializers
}  // namespace fetch
