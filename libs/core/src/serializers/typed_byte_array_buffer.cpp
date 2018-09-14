//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/serializers/typed_byte_array_buffer.hpp"


namespace fetch {
namespace serializers {

template<>
void TypedByteArrayBuffer::ReadBytes(uint8_t *arr, std::size_t const &size)
{
  if (int64_t(size) > bytes_left())
  {
    throw SerializableException(
        error::TYPE_ERROR, "Typed serializer error (ReadBytes): Not enough bytes " +
                               std::to_string(bytes_left()) + " not  " + std::to_string(size));
  }

  for (std::size_t i = 0; i < size; ++i)
  {
    arr[i] = data_[pos_++];
  }
}

template<>
void TypedByteArrayBuffer::ReadByteArray(byte_array::ConstByteArray &b, std::size_t const &size)
{
  if (int64_t(size) > bytes_left())
  {
    throw SerializableException(
        error::TYPE_ERROR, "Typed serializer error (ReadByteArray): Not enough bytes " +
                               std::to_string(bytes_left()) + " not  " + std::to_string(size));
  }

  b = data_.SubArray(pos_, size);
  pos_ += size;
}

}  // namespace serializers
}  // namespace fetch
