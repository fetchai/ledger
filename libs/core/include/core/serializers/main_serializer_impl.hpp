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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/array_interface.hpp"
#include "core/serializers/binary_interface.hpp"
#include "core/serializers/container_constructor_interface.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/map_interface.hpp"
#include "vectorise/platform.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

template <typename WriteType, typename InitialType>
void MsgPackByteArrayBuffer::WritePrimitive(InitialType const &val)
{
  WriteType w = static_cast<WriteType>(val);
  WriteBytes(reinterpret_cast<uint8_t const *>(&w), sizeof(w));
}

template <typename ReadType, typename FinalType>
void MsgPackByteArrayBuffer::ReadPrimitive(FinalType &val)
{
  ReadType r;
  ReadBytes(reinterpret_cast<uint8_t *>(&r), sizeof(r));
  val = static_cast<FinalType>(r);
}

template <typename T>
typename IgnoredSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                   operator<<(T const &)
{
  return *this;
}

template <typename T>
typename IgnoredSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                   operator>>(T &)
{
  return *this;
}

template <typename T>
typename ForwardSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                   operator<<(T const &val)
{
  try
  {
    ForwardSerializer<T, MsgPackByteArrayBuffer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ForwardSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                   operator>>(T &val)
{
  try
  {
    ForwardSerializer<T, MsgPackByteArrayBuffer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename IntegerSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                   operator<<(T const &val)
{
  try
  {
    IntegerSerializer<T, MsgPackByteArrayBuffer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename IntegerSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                   operator>>(T &val)
{
  try
  {
    IntegerSerializer<T, MsgPackByteArrayBuffer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename FloatSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::operator<<(
    T const &val)
{
  try
  {
    FloatSerializer<T, MsgPackByteArrayBuffer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename FloatSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::operator>>(
    T &val)
{
  try
  {
    FloatSerializer<T, MsgPackByteArrayBuffer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BooleanSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                   operator<<(T const &val)
{
  try
  {
    BooleanSerializer<T, MsgPackByteArrayBuffer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BooleanSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                   operator>>(T &val)
{
  try
  {
    BooleanSerializer<T, MsgPackByteArrayBuffer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename StringSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                  operator<<(T const &val)
{
  try
  {
    StringSerializer<T, MsgPackByteArrayBuffer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename StringSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                  operator>>(T &val)
{
  try
  {
    StringSerializer<T, MsgPackByteArrayBuffer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }
  return *this;
}

template <typename T>
typename BinarySerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                  operator<<(T const &val)
{
  using Serializer = BinarySerializer<T, MsgPackByteArrayBuffer>;
  using Constructor =
      interfaces::BinaryConstructorInterface<TypeCodes::BINARY_CODE_FIXED, TypeCodes::BINARY_CODE16,
                                             TypeCodes::BINARY_CODE32>;

  try
  {
    Constructor constructor(*this);
    Serializer::Serialize(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BinarySerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::
                                                                  operator>>(T &val)
{
  using Serializer = BinarySerializer<T, MsgPackByteArrayBuffer>;
  try
  {
    interfaces::BinaryDeserializer stream(*this);
    Serializer::Deserialize(stream, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ArraySerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::operator<<(
    T const &val)
{
  using Serializer = ArraySerializer<T, MsgPackByteArrayBuffer>;
  using Constructor =
      interfaces::ContainerConstructorInterface<interfaces::ArrayInterface,
                                                TypeCodes::ARRAY_CODE_FIXED,
                                                TypeCodes::ARRAY_CODE16, TypeCodes::ARRAY_CODE32>;

  try
  {
    Constructor constructor(*this);
    Serializer::Serialize(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ArraySerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::operator>>(
    T &val)
{
  using Serializer = ArraySerializer<T, MsgPackByteArrayBuffer>;
  try
  {
    interfaces::ArrayDeserializer array(*this);
    Serializer::Deserialize(array, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename MapSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::operator<<(
    T const &val)
{
  using Serializer = MapSerializer<T, MsgPackByteArrayBuffer>;
  using Constructor =
      interfaces::ContainerConstructorInterface<interfaces::MapInterface, TypeCodes::MAP_CODE_FIXED,
                                                TypeCodes::MAP_CODE16, TypeCodes::MAP_CODE32>;

  try
  {
    Constructor constructor(*this);
    Serializer::Serialize(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename MapSerializer<T, MsgPackByteArrayBuffer>::DriverType &MsgPackByteArrayBuffer::operator>>(
    T &val)
{
  using Serializer = MapSerializer<T, MsgPackByteArrayBuffer>;
  try
  {
    interfaces::MapDeserializer map(*this);
    Serializer::Deserialize(map, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
MsgPackByteArrayBuffer &MsgPackByteArrayBuffer::Pack(T const *val)
{
  return this->operator<<(val);
}

template <typename T>
MsgPackByteArrayBuffer &MsgPackByteArrayBuffer::Pack(T const &val)
{
  return this->operator<<(val);
}

template <typename T>
MsgPackByteArrayBuffer &MsgPackByteArrayBuffer::Unpack(T &val)
{
  return this->operator>>(val);
}

template <typename... ARGS>
MsgPackByteArrayBuffer &MsgPackByteArrayBuffer::Append(ARGS const &... args)
{
  auto size_count_guard = sizeCounterGuardFactory(size_counter_);
  if (size_count_guard.is_unreserved())
  {
    size_counter_.Allocate(size());
    size_counter_.seek(tell());

    size_counter_.Append(args...);
    if (size() < size_counter_.size())
    {
      Reserve(size_counter_.size() - size());
    }
  }

  AppendInternal(args...);
  return *this;
}

template <typename T, typename... ARGS>
void MsgPackByteArrayBuffer::AppendInternal(T const &arg, ARGS const &... args)
{
  *this << arg;
  AppendInternal(args...);
}

}  // namespace serializers
}  // namespace fetch
