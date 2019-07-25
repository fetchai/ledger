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
template <typename I, uint8_t CF, uint8_t C16, uint8_t C32>
class ContainerConstructorInterface
{
public:
  using Type = I;

  enum
  {
    CODE_FIXED = CF,
    CODE16     = C16,
    CODE32     = C32
  };

  ContainerConstructorInterface(MsgPackByteArrayBuffer &serializer)
    : serializer_{serializer}
  {}

  Type operator()(uint64_t count)
  {
    if (created_)
    {
      throw SerializableException(std::string("Constructor is one time use only."));
    }

    uint8_t opcode = 0;
    if (count < 16)
    {
      opcode = static_cast<uint8_t>(CODE_FIXED | (count & 0xF));
      opcode = platform::ToBigEndian(opcode);
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));
    }
    else if (count < (1 << 16))
    {
      opcode = static_cast<uint8_t>(CODE16);
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));

      uint16_t size = static_cast<uint16_t>(count);
      size          = platform::ToBigEndian(size);
      serializer_.Allocate(sizeof(size));
      serializer_.WriteBytes(reinterpret_cast<uint8_t *>(&size), sizeof(size));
    }
    else if (count < (1ull << 32))
    {
      opcode = static_cast<uint8_t>(CODE32);
      serializer_.Allocate(sizeof(opcode));
      serializer_.WriteBytes(&opcode, sizeof(opcode));

      uint32_t size = static_cast<uint32_t>(count);
      size          = platform::ToBigEndian(size);
      serializer_.Allocate(sizeof(size));
      serializer_.WriteBytes(reinterpret_cast<uint8_t *>(&size), sizeof(size));
    }
    else
    {
      throw SerializableException(
          error::TYPE_ERROR,
          std::string("Cannot create container type with more than 1 << 32 elements"));
    }

    created_ = true;
    return Type(serializer_, count);
  }

  MsgPackByteArrayBuffer &serializer()
  {
    return serializer_;
  }

private:
  bool                    created_{false};
  MsgPackByteArrayBuffer &serializer_;
};

}  // namespace interfaces
}  // namespace serializers
}  // namespace fetch
