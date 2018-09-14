#pragma once
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

#include "core/assert.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/type_register.hpp"
#include <type_traits>

namespace fetch {
namespace serializers {

using typed_array_buffer_id_type = TypeRegister<void>;
using TypedByteArrayBuffer = ByteArrayBufferEx<typed_array_buffer_id_type>;

template<>
template<typename T>
TypedByteArrayBuffer &TypedByteArrayBuffer::operator<<(T const *val)
{
  Serialize(*this, TypeRegister<void>::value_type(TypeRegister<T const *>::value));
  Serialize(*this, val);
  return *this;
}

template<>
template<typename T>
TypedByteArrayBuffer &TypedByteArrayBuffer::operator<<(T const &val)
{
  Serialize(*this, TypeRegister<void>::value_type(TypeRegister<T>::value));
  Serialize(*this, val);
  return *this;
}

template<>
template<typename T>
TypedByteArrayBuffer &TypedByteArrayBuffer::operator>>(T &val)
{
  TypeRegister<void>::value_type type;
  Deserialize(*this, type);
  if (TypeRegister<T>::value != type)
  {
    fetch::logger.Debug("Serializer at position ", pos_, " out of ", data_.size());
    fetch::logger.Error(byte_array_type("Expected type '") + TypeRegister<T>::name() +
                        byte_array_type("' differs from deserialized type '") +
                        ErrorCodeToMessage(type) + byte_array_type("'"));

    throw SerializableException(error::TYPE_ERROR,
                                byte_array_type("Expected type '") + TypeRegister<T>::name() +
                                    byte_array_type("' differs from deserialized type '") +
                                    ErrorCodeToMessage(type) + byte_array_type("'"));
  }
  Deserialize(*this, val);
  return *this;
}

template<>
template<typename T>
SizeCounter<TypedByteArrayBuffer> & SizeCounter<TypedByteArrayBuffer>::operator<<(T const &val)
{
  Serialize(*this, TypeRegister<void>::value_type(TypeRegister<T>::value));
  Serialize(*this, val);
  return *this;
}

}  // namespace serializers
}  // namespace fetch
