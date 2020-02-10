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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/serialisers/counter.hpp"
#include "core/serialisers/exception.hpp"
#include "core/serialisers/group_definitions.hpp"
#include "core/serialisers/main_serialiser.hpp"
#include "vectorise/platform.hpp"

#include <type_traits>

namespace fetch {
namespace serialisers {

MsgPackSerialiser::MsgPackSerialiser(byte_array::ByteArray s)
  : data_{std::move(s)}
{}

MsgPackSerialiser::MsgPackSerialiser(MsgPackSerialiser const &from)
  : data_{from.data_.Copy()}
  , pos_{from.pos_}
  , size_counter_{from.size_counter_}
{}

MsgPackSerialiser &MsgPackSerialiser::operator=(MsgPackSerialiser const &from)
{
  *this = MsgPackSerialiser{from};
  return *this;
}

void MsgPackSerialiser::WriteNil()
{
  Allocate(sizeof(uint8_t));
  WriteByte(static_cast<uint8_t>(TypeCodes::NIL));
}

void MsgPackSerialiser::Allocate(uint64_t const &delta)
{
  Resize(delta, ResizeParadigm::RELATIVE);
}

void MsgPackSerialiser::Resize(uint64_t const &size, ResizeParadigm const &resize_paradigm,
                               bool const zero_reserved_space)
{
  data_.Resize(size, resize_paradigm, zero_reserved_space);

  switch (resize_paradigm)
  {
  case ResizeParadigm::RELATIVE:
    break;

  case ResizeParadigm::ABSOLUTE:
    if (pos_ > size)
    {
      seek(size);
    }
    break;
  };
}

MsgPackSerialiser::ArrayConstructor MsgPackSerialiser::NewArrayConstructor()
{
  return ArrayConstructor(*this);
}

MsgPackSerialiser::ArrayDeserialiser MsgPackSerialiser::NewArrayDeserialiser()
{
  return ArrayDeserialiser(*this);
}

MsgPackSerialiser::MapConstructor MsgPackSerialiser::NewMapConstructor()
{
  return MapConstructor(*this);
}

MsgPackSerialiser::MapDeserialiser MsgPackSerialiser::NewMapDeserialiser()
{
  return MapDeserialiser(*this);
}

MsgPackSerialiser::PairConstructor MsgPackSerialiser::NewPairConstructor()
{
  return PairConstructor(*this);
}

MsgPackSerialiser::PairDeserialiser MsgPackSerialiser::NewPairDeserialiser()
{
  return PairDeserialiser(*this);
}

void MsgPackSerialiser::Reserve(uint64_t const &size, ResizeParadigm const &resize_paradigm,
                                bool const zero_reserved_space)
{
  data_.Reserve(size, resize_paradigm, zero_reserved_space);
}

void MsgPackSerialiser::WriteBytes(uint8_t const *arr, uint64_t const &size)
{
  data_.WriteBytes(arr, size, pos_);
  pos_ += size;
}

void MsgPackSerialiser::WriteByte(uint8_t const &val)
{
  data_.WriteBytes(&val, 1, pos_);
  ++pos_;
}

void MsgPackSerialiser::ReadByte(uint8_t &val)
{
  data_.ReadBytes(&val, 1, pos_);
  ++pos_;
}

void MsgPackSerialiser::ReadBytes(uint8_t *arr, uint64_t const &size)
{
  if (size + pos_ > data_.size())
  {
    throw std::runtime_error("Attempted read exceeds buffer size.");
  }

  data_.ReadBytes(arr, size, pos_);
  pos_ += size;
}

void MsgPackSerialiser::ReadByteArray(byte_array::ConstByteArray &b, uint64_t const &size)
{
  if (size + pos_ > data_.size())
  {
    throw std::runtime_error("Attempted read exceeds buffer size.");
  }

  b = data_.SubArray(pos_, size);
  pos_ += size;
}

void MsgPackSerialiser::SkipBytes(uint64_t const &size)
{
  pos_ += size;
}

void MsgPackSerialiser::seek(uint64_t p)
{
  pos_ = p;
}

uint64_t MsgPackSerialiser::tell() const
{
  return pos_;
}

uint64_t MsgPackSerialiser::size() const
{
  return data_.size();
}

uint64_t MsgPackSerialiser::capacity() const
{
  return data_.capacity();
}

int64_t MsgPackSerialiser::bytes_left() const
{
  return static_cast<int64_t>(data_.size()) - static_cast<int64_t>(pos_);
}

byte_array::ByteArray const &MsgPackSerialiser::data() const
{
  return data_;
}

void MsgPackSerialiser::AppendInternal()
{}

}  // namespace serialisers
}  // namespace fetch
