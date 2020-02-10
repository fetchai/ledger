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
#include "core/serialisers/array_interface.hpp"
#include "core/serialisers/binary_interface.hpp"
#include "core/serialisers/container_constructor_interface.hpp"
#include "core/serialisers/counter.hpp"
#include "core/serialisers/exception.hpp"
#include "core/serialisers/group_definitions.hpp"
#include "core/serialisers/main_serialiser_definition.hpp"
#include "core/serialisers/map_interface.hpp"
#include "core/serialisers/pair_interface.hpp"
#include "vectorise/platform.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace fetch {
namespace serialisers {

/**
 * When serializing large objects it is useful to use this wrapper class
 * It ensures the right amount of memory is allocated first
 */
class LargeObjectSerialiseHelper
{
public:
  LargeObjectSerialiseHelper() = default;

  explicit LargeObjectSerialiseHelper(fetch::byte_array::ConstByteArray buf)
    : buffer{std::move(buf)}
  {}
  LargeObjectSerialiseHelper(LargeObjectSerialiseHelper const &)     = delete;
  LargeObjectSerialiseHelper(LargeObjectSerialiseHelper &&) noexcept = delete;

  template <typename T>
  void operator<<(T const &large_object)
  {
    Serialise(large_object);
  }

  template <typename T>
  void operator>>(T &large_object)
  {
    Deserialise(large_object);
  }

  template <typename T>
  void Serialise(T const &large_object)
  {
    counter << large_object;
    buffer.Reserve(counter.size());
    buffer << large_object;
  }

  template <typename T>
  void Deserialise(T &large_object)
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
  LargeObjectSerialiseHelper &operator=(LargeObjectSerialiseHelper const &) = delete;
  LargeObjectSerialiseHelper &operator=(LargeObjectSerialiseHelper &&) = delete;

private:
  MsgPackSerialiser buffer{};
  SizeCounter       counter{};
};

template <typename WriteType, typename InitialType>
void MsgPackSerialiser::WritePrimitive(InitialType const &val)
{
  auto w = static_cast<WriteType>(val);
  WriteBytes(reinterpret_cast<uint8_t const *>(&w), sizeof(w));
}

template <typename ReadType, typename FinalType>
void MsgPackSerialiser::ReadPrimitive(FinalType &val)
{
  ReadType r;
  ReadBytes(reinterpret_cast<uint8_t *>(&r), sizeof(r));
  val = static_cast<FinalType>(r);
}

template <typename T>
typename IgnoredSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const & /*unused*/)
{
  return *this;
}

template <typename T>
typename IgnoredSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(
    T & /*unused*/)
{
  return *this;
}

template <typename T>
typename ForwardSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const &val)
{
  try
  {
    ForwardSerialiser<T, MsgPackSerialiser>::Serialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ForwardSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(T &val)
{
  try
  {
    ForwardSerialiser<T, MsgPackSerialiser>::Deserialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename IntegerSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const &val)
{
  try
  {
    IntegerSerialiser<T, MsgPackSerialiser>::Serialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename IntegerSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(T &val)
{
  try
  {
    IntegerSerialiser<T, MsgPackSerialiser>::Deserialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename FloatSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const &val)
{
  try
  {
    FloatSerialiser<T, MsgPackSerialiser>::Serialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename FloatSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(T &val)
{
  try
  {
    FloatSerialiser<T, MsgPackSerialiser>::Deserialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BooleanSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const &val)
{
  try
  {
    BooleanSerialiser<T, MsgPackSerialiser>::Serialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BooleanSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(T &val)
{
  try
  {
    BooleanSerialiser<T, MsgPackSerialiser>::Deserialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename StringSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const &val)
{
  try
  {
    StringSerialiser<T, MsgPackSerialiser>::Serialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename StringSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(T &val)
{
  try
  {
    StringSerialiser<T, MsgPackSerialiser>::Deserialise(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }
  return *this;
}

template <typename T>
typename BinarySerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const &val)
{
  using Serialiser = BinarySerialiser<T, MsgPackSerialiser>;
  using Constructor =
      interfaces::BinaryConstructorInterface<MsgPackSerialiser, TypeCodes::BINARY_CODE8,
                                             TypeCodes::BINARY_CODE16, TypeCodes::BINARY_CODE32>;

  try
  {
    Constructor constructor(*this);
    Serialiser::Serialise(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BinarySerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(T &val)
{
  using Serialiser = BinarySerialiser<T, MsgPackSerialiser>;
  try
  {
    interfaces::BinaryDeserialiser<MsgPackSerialiser> stream(*this);
    Serialiser::Deserialise(stream, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ArraySerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const &val)
{
  using Serialiser = ArraySerialiser<T, MsgPackSerialiser>;

  try
  {
    ArrayConstructor constructor(*this);
    Serialiser::Serialise(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ArraySerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(T &val)
{
  using Serialiser = ArraySerialiser<T, MsgPackSerialiser>;
  try
  {
    ArrayDeserialiser array(*this);
    Serialiser::Deserialise(array, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename MapSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator<<(
    T const &val)
{
  using Serialiser = MapSerialiser<T, MsgPackSerialiser>;

  try
  {
    MapConstructor constructor(*this);
    Serialiser::Serialise(constructor, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename MapSerialiser<T, MsgPackSerialiser>::DriverType &MsgPackSerialiser::operator>>(T &val)
{
  using Serialiser = MapSerialiser<T, MsgPackSerialiser>;
  try
  {
    MapDeserialiser map(*this);
    Serialiser::Deserialise(map, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
MsgPackSerialiser &MsgPackSerialiser::Pack(T const *val)
{
  return this->operator<<(val);
}

template <typename T>
MsgPackSerialiser &MsgPackSerialiser::Pack(T const &val)
{
  return this->operator<<(val);
}

template <typename T>
MsgPackSerialiser &MsgPackSerialiser::Unpack(T &val)
{
  return this->operator>>(val);
}

template <typename... ARGS>
MsgPackSerialiser &MsgPackSerialiser::Append(ARGS const &... args)
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
void MsgPackSerialiser::AppendInternal(T const &arg, ARGS const &... args)
{
  *this << arg;
  AppendInternal(args...);
}

}  // namespace serialisers
}  // namespace fetch
