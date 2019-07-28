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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"
#include "vectorise/platform.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

MsgPackSerializer::MsgPackSerializer(byte_array::ByteArray const &s)
  : data_{s.Copy()}
{}

MsgPackSerializer::MsgPackSerializer(MsgPackSerializer const &from)
  : data_{from.data_.Copy()}
  , pos_{from.pos_}
  , size_counter_{from.size_counter_}
{}

MsgPackSerializer &MsgPackSerializer::operator=(MsgPackSerializer const &from)
{
  *this = MsgPackSerializer{from};
  return *this;
}

void MsgPackSerializer::Allocate(uint64_t const &delta)
{
  Resize(delta, ResizeParadigm::RELATIVE);
}

void MsgPackSerializer::Resize(uint64_t const &size, ResizeParadigm const &resize_paradigm,
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

void MsgPackSerializer::Reserve(uint64_t const &size, ResizeParadigm const &resize_paradigm,
                                bool const zero_reserved_space)
{
  data_.Reserve(size, resize_paradigm, zero_reserved_space);
}

void MsgPackSerializer::WriteBytes(uint8_t const *arr, uint64_t const &size)
{
  data_.WriteBytes(arr, size, pos_);
  pos_ += size;
}

void MsgPackSerializer::WriteByte(uint8_t const &val)
{
  data_.WriteBytes(&val, 1, pos_);
  ++pos_;
}

void MsgPackSerializer::ReadByte(uint8_t &val)
{
  data_.ReadBytes(&val, 1, pos_);
  ++pos_;
}

void MsgPackSerializer::ReadBytes(uint8_t *arr, uint64_t const &size)
{
#ifndef NDEBUG
  if (size + pos_ > data_.size())
  {
    throw std::runtime_error("Attempted read exceeds buffer size.");
  }
#endif
  data_.ReadBytes(arr, size, pos_);
  pos_ += size;
}

void MsgPackSerializer::ReadByteArray(byte_array::ConstByteArray &b, uint64_t const &size)
{
#ifndef NDEBUG
  if (size + pos_ > data_.size())
  {
    throw std::runtime_error("Attempted read exceeds buffer size.");
  }
#endif
  b = data_.SubArray(pos_, size);
  pos_ += size;
}

void MsgPackSerializer::SkipBytes(uint64_t const &size)
{
  pos_ += size;
}

void MsgPackSerializer::seek(uint64_t p)
{
  pos_ = p;
}

uint64_t MsgPackSerializer::tell() const
{
  return pos_;
}

uint64_t MsgPackSerializer::size() const
{
  return data_.size();
}

uint64_t MsgPackSerializer::capacity() const
{
  return data_.capacity();
}

int64_t MsgPackSerializer::bytes_left() const
{
  return static_cast<int64_t>(data_.size()) - static_cast<int64_t>(pos_);
}

byte_array::ByteArray const &MsgPackSerializer::data() const
{
  return data_;
}

void MsgPackSerializer::AppendInternal()
{}

}  // namespace serializers
}  // namespace fetch
