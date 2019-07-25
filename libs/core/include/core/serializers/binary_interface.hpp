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

namespace fetch {
namespace serializers {
namespace interfaces {
class BinaryInterface
{
public:
  BinaryInterface(MsgPackByteArrayBuffer &serializer, uint64_t size)
    : serializer_{serializer}
    , size_{std::move(size)}
  {}

  void Write(uint8_t const *arr, uint64_t partial_size)
  {
    pos_ += partial_size;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("exceeded number of allocated elements in array serialization"));
    }

    serializer_.WriteBytes(arr, partial_size);
  }

private:
  MsgPackByteArrayBuffer &serializer_;
  uint64_t                size_;
  uint64_t                pos_{0};
};

template <uint8_t C8, uint8_t C16, uint8_t C32>
class BinaryConstructorInterface
{
public:
  enum
  {
    CODE8  = C8,
    CODE16 = C16,
    CODE32 = C32
  };

  BinaryConstructorInterface(MsgPackByteArrayBuffer &serializer)
    : serializer_{serializer}
  {}

  BinaryInterface operator()(uint64_t count)
  {
    if (created_)
    {
      throw SerializableException(std::string("Constructor is one time use only."));
    }

    if (count < (1 << 8))
    {
      uint8_t opcode = static_cast<uint8_t>(CODE8);
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));

      uint8_t size = static_cast<uint8_t>(count);
      size = platform::ToBigEndian(size);      
      serializer_.Allocate(sizeof(size));
      serializer_.WriteBytes(&size, sizeof(size));
    }
    else if (count < (1 << 16))
    {
      uint8_t opcode = static_cast<uint8_t>(CODE16);
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));

      uint16_t size = static_cast<uint16_t>(count);
      size = platform::ToBigEndian(size);      
      serializer_.Allocate(sizeof(size));
      serializer_.WriteBytes(reinterpret_cast<uint8_t *>(&size), sizeof(size));
    }
    else if (count < (1ull << 32))
    {
      uint8_t opcode = static_cast<uint8_t>(CODE32);
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));

      uint32_t size = static_cast<uint32_t>(count);
      size = platform::ToBigEndian(size);      
      serializer_.Allocate(sizeof(count));
      serializer_.WriteBytes(reinterpret_cast<uint8_t *>(&count), sizeof(count));
    }
    else
    {
      throw SerializableException(
          error::TYPE_ERROR,
          std::string("Cannot create container type with more than 1 << 32 elements"));
    }

    created_ = true;
    return BinaryInterface(serializer_, count);
  }

private:
  bool                    created_{false};
  MsgPackByteArrayBuffer &serializer_;
};

class BinaryDeserializer
{
public:
  enum
  {
    CODE8  = 0xc4, // TODO(tfr): Needs to be move to central place
    CODE16 = 0xc5,
    CODE32 = 0xc6
  };
  BinaryDeserializer(MsgPackByteArrayBuffer &serializer)
    : serializer_{serializer}
  {
    uint8_t  opcode;
    uint32_t size;
    serializer_.ReadByte(opcode);
    switch (opcode)
    {
    case CODE8:  
    {
      uint8_t tmp;
      serializer_.ReadBytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp));
      tmp = platform::FromBigEndian(tmp);
      size = static_cast< uint32_t >(tmp);
      break;
    }
    case CODE16:
    {
      uint16_t tmp;
      serializer_.ReadBytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp));
      tmp = platform::FromBigEndian(tmp);
      size = static_cast< uint32_t >(tmp);
      break;
    }
    case CODE32:
    {
      uint32_t tmp;
      serializer_.ReadBytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp));
      tmp = platform::FromBigEndian(tmp);
      size = static_cast< uint32_t >(tmp);
      break;
    }
    default:
      throw SerializableException(std::string("incorrect size opcode for binary stream size."));
    }

    size_ = static_cast<uint64_t>(size);
  }

  void Read(uint8_t *arr, uint64_t size)
  {
    pos_ += size;
    if (pos_ > size_)
    {
      throw SerializableException(
          std::string("tried to deserialise more fields in map than there exists."));
    }
    serializer_.ReadBytes(arr, size);
  }

  uint64_t size() const
  {
    return size_;
  }

private:
  MsgPackByteArrayBuffer &serializer_;
  uint64_t                size_{0};
  uint64_t                pos_{0};
};
}  // namespace interfaces
}  // namespace serializers
}  // namespace fetch
