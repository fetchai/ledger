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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/array_interface.hpp"
#include "core/serializers/binary_interface.hpp"
#include "core/serializers/container_constructor_interface.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer_definition.hpp"
#include "core/serializers/map_interface.hpp"
#include "core/serializers/pair_interface.hpp"
#include "vectorise/platform.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace fetch {
namespace serializers {

/**
 * When serializing large objects it is useful to use this wrapper class
 * It ensures the right amount of memory is allocated first
 */
class LargeObjectSerializeHelper
{
public:
  LargeObjectSerializeHelper() = default;

  explicit LargeObjectSerializeHelper(fetch::byte_array::ConstByteArray buf)
    : buffer{std::move(buf)}
  {}
  LargeObjectSerializeHelper(LargeObjectSerializeHelper const &)     = delete;
  LargeObjectSerializeHelper(LargeObjectSerializeHelper &&) noexcept = delete;

  template <typename T>
  void operator<<(T const &large_object)
  {
    Serialize(large_object);
  }

  template <typename T>
  void operator>>(T &large_object)
  {
    Deserialize(large_object);
  }

  template <typename T>
  void Serialize(T const &large_object)
  {
    counter << large_object;
    buffer.Reserve(counter.size());
    buffer << large_object;
  }

  template <typename T>
  void Deserialize(T &large_object)
  {
    buffer.seek(0);
    buffer >> large_object;
  }

  byte_array::ConstByteArray const &data() const
  {
    return buffer.data();
  }

  std::size_t size() const
  {
    return buffer.size();
  }

  // Operators
  LargeObjectSerializeHelper &operator=(LargeObjectSerializeHelper const &) = delete;
  LargeObjectSerializeHelper &operator=(LargeObjectSerializeHelper &&) = delete;

private:
  MsgPackSerializer buffer{};
  SizeCounter       counter{};
};

template <typename WriteType, typename InitialType>
void MsgPackSerializer::WritePrimitive(InitialType const &val)
{
  auto w = static_cast<WriteType>(val);
  WriteBytes(reinterpret_cast<uint8_t const *>(&w), sizeof(w));
}

template <typename ReadType, typename FinalType>
void MsgPackSerializer::ReadPrimitive(FinalType &val)
{
  ReadType r;
  ReadBytes(reinterpret_cast<uint8_t *>(&r), sizeof(r));
  val = static_cast<FinalType>(r);
}

template <typename T>
typename IgnoredSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const & /*unused*/)
{
  return *this;
}

template <typename T>
typename IgnoredSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(
    T & /*unused*/)
{
  return *this;
}

template <typename T>
typename ForwardSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const &val)
{
  try
  {
    ForwardSerializer<T, MsgPackSerializer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ForwardSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(T &val)
{
  try
  {
    ForwardSerializer<T, MsgPackSerializer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename IntegerSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const &val)
{
  try
  {
    IntegerSerializer<T, MsgPackSerializer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename IntegerSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(T &val)
{
  try
  {
    IntegerSerializer<T, MsgPackSerializer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename FloatSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const &val)
{
  try
  {
    FloatSerializer<T, MsgPackSerializer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename FloatSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(T &val)
{
  try
  {
    FloatSerializer<T, MsgPackSerializer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BooleanSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const &val)
{
  try
  {
    BooleanSerializer<T, MsgPackSerializer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BooleanSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(T &val)
{
  try
  {
    BooleanSerializer<T, MsgPackSerializer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename StringSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const &val)
{
  try
  {
    StringSerializer<T, MsgPackSerializer>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename StringSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(T &val)
{
  try
  {
    StringSerializer<T, MsgPackSerializer>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }
  return *this;
}

template <typename T>
typename BinarySerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const &val)
{
  using Serializer = BinarySerializer<T, MsgPackSerializer>;
  using Constructor =
      interfaces::BinaryConstructorInterface<MsgPackSerializer, TypeCodes::BINARY_CODE8,
                                             TypeCodes::BINARY_CODE16, TypeCodes::BINARY_CODE32>;

  try
  {
    Constructor constructor(*this);
    Serializer::Serialize(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BinarySerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(T &val)
{
  using Serializer = BinarySerializer<T, MsgPackSerializer>;
  try
  {
    interfaces::BinaryDeserializer<MsgPackSerializer> stream(*this);
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
typename ArraySerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const &val)
{
  using Serializer = ArraySerializer<T, MsgPackSerializer>;

  try
  {
    ArrayConstructor constructor(*this);
    Serializer::Serialize(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ArraySerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(T &val)
{
  using Serializer = ArraySerializer<T, MsgPackSerializer>;
  try
  {
    ArrayDeserializer array(*this);
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
typename MapSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator<<(
    T const &val)
{
  using Serializer = MapSerializer<T, MsgPackSerializer>;

  try
  {
    MapConstructor constructor(*this);
    Serializer::Serialize(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename MapSerializer<T, MsgPackSerializer>::DriverType &MsgPackSerializer::operator>>(T &val)
{
  using Serializer = MapSerializer<T, MsgPackSerializer>;
  try
  {
    MapDeserializer map(*this);
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
MsgPackSerializer &MsgPackSerializer::Pack(T const *val)
{
  return this->operator<<(val);
}

template <typename T>
MsgPackSerializer &MsgPackSerializer::Pack(T const &val)
{
  return this->operator<<(val);
}

template <typename T>
MsgPackSerializer &MsgPackSerializer::Unpack(T &val)
{
  return this->operator>>(val);
}

template <typename... ARGS>
MsgPackSerializer &MsgPackSerializer::Append(ARGS const &... args)
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
void MsgPackSerializer::AppendInternal(T const &arg, ARGS const &... args)
{
  *this << arg;
  AppendInternal(args...);
}

}  // namespace serializers
}  // namespace fetch
