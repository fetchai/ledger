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
#include "core/macros.hpp"
#include "core/serializers/array_interface.hpp"
#include "core/serializers/binary_interface.hpp"
#include "core/serializers/container_constructor_interface.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/map_interface.hpp"
#include "core/serializers/pair_interface.hpp"
#include "vectorise/platform.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

namespace fetch {
namespace serializers {

class SizeCounter
{
public:
  using SelfType = SizeCounter;
  /// Array Helpers
  /// @{
  using ArrayConstructor = interfaces::ContainerConstructorInterface<
      SizeCounter, interfaces::ArrayInterface<SizeCounter>, TypeCodes::ARRAY_CODE_FIXED,
      TypeCodes::ARRAY_CODE16, TypeCodes::ARRAY_CODE32>;
  /// @}

  /// Map Helpers
  /// @{
  using MapConstructor =
      interfaces::ContainerConstructorInterface<SizeCounter, interfaces::MapInterface<SizeCounter>,
                                                TypeCodes::MAP_CODE_FIXED, TypeCodes::MAP_CODE16,
                                                TypeCodes::MAP_CODE32>;
  /// @}

  void Allocate(std::size_t delta)
  {
    Resize(delta, ResizeParadigm::RELATIVE);
  }

  void Resize(std::size_t size, ResizeParadigm const &resize_paradigm = ResizeParadigm::RELATIVE,
              bool const zero_reserved_space = true)
  {
    FETCH_UNUSED(zero_reserved_space);

    Reserve(size, resize_paradigm);

    switch (resize_paradigm)
    {
    case ResizeParadigm::RELATIVE:
      size_ += size;
      break;

    case ResizeParadigm::ABSOLUTE:
      size_ = size;
      if (pos_ > size)
      {
        seek(size_);
      }
      break;
    };
  }

  void Reserve(std::size_t size, ResizeParadigm const &resize_paradigm = ResizeParadigm::RELATIVE,
               bool const zero_reserved_space = true)
  {
    FETCH_UNUSED(zero_reserved_space);

    switch (resize_paradigm)
    {
    case ResizeParadigm::RELATIVE:
      reserved_size_ += size;
      break;

    case ResizeParadigm::ABSOLUTE:
      if (reserved_size_ < size)
      {
        reserved_size_ = size;
      }
      break;
    };
  }

  void WriteByte(uint8_t /*unused*/)
  {
    ++pos_;
  }

  void WriteBytes(uint8_t const * /*unused*/, std::size_t size)
  {
    pos_ += size;
  }

  void SkipBytes(std::size_t size)
  {
    pos_ += size;
  }

  void WriteNil()
  {
    ++pos_;
  }

  template <typename T>
  typename IgnoredSerializer<T, SizeCounter>::DriverType &operator<<(T const & /*unused*/);

  template <typename T>
  typename IgnoredSerializer<T, SizeCounter>::DriverType &operator>>(T & /*unused*/);

  template <typename T>
  typename ForwardSerializer<T, SizeCounter>::DriverType &operator<<(T const &val);

  template <typename T>
  typename ForwardSerializer<T, SizeCounter>::DriverType &operator>>(T &val);

  template <typename T>
  typename IntegerSerializer<T, SizeCounter>::DriverType &operator<<(T const &val);

  template <typename T>
  typename IntegerSerializer<T, SizeCounter>::DriverType &operator>>(T &val);

  template <typename T>
  typename FloatSerializer<T, SizeCounter>::DriverType &operator<<(T const &val);

  template <typename T>
  typename FloatSerializer<T, SizeCounter>::DriverType &operator>>(T &val);

  template <typename T>
  typename BooleanSerializer<T, SizeCounter>::DriverType &operator<<(T const &val);

  template <typename T>
  typename BooleanSerializer<T, SizeCounter>::DriverType &operator>>(T &val);

  template <typename T>
  typename StringSerializer<T, SizeCounter>::DriverType &operator<<(T const &val);

  template <typename T>
  typename StringSerializer<T, SizeCounter>::DriverType &operator>>(T &val);

  template <typename T>
  typename BinarySerializer<T, SizeCounter>::DriverType &operator<<(T const &val);

  template <typename T>
  typename BinarySerializer<T, SizeCounter>::DriverType &operator>>(T &val);

  ArrayConstructor NewArrayConstructor();

  template <typename T>
  typename ArraySerializer<T, SizeCounter>::DriverType &operator<<(T const &val);

  template <typename T>
  typename ArraySerializer<T, SizeCounter>::DriverType &operator>>(T &val);

  MapConstructor NewMapConstructor();

  template <typename T>
  typename MapSerializer<T, SizeCounter>::DriverType &operator<<(T const &val);

  template <typename T>
  typename MapSerializer<T, SizeCounter>::DriverType &operator>>(T &val);

  template <typename T>
  SelfType &Pack(T const *val)
  {
    return this->operator<<(val);
  }

  template <typename T>
  SelfType &Pack(T const &val)
  {
    return this->operator<<(val);
  }

  void seek(std::size_t p)
  {
    pos_ = p;
  }

  std::size_t tell() const
  {
    return pos_;
  }

  std::size_t size() const
  {
    return size_;
  }

  std::size_t capacity() const
  {
    return reserved_size_;
  }

  int64_t bytes_left() const
  {
    return int64_t(size_) - int64_t(pos_);
  }

  template <typename... ARGS>
  SelfType &Append(ARGS const &... args)
  {
    AppendInternal(args...);
    return *this;
  }

private:
  template <typename T, typename... ARGS>
  void AppendInternal(T const &arg, ARGS const &... args)
  {
    *this << arg;
    AppendInternal(args...);
  }

  void AppendInternal()
  {}

  std::size_t size_          = 0;
  std::size_t pos_           = 0;
  std::size_t reserved_size_ = 0;
};

template <typename T>
typename IgnoredSerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(
    T const & /*unused*/)
{
  return *this;
}

template <typename T>
typename IgnoredSerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T & /*unused*/)
{
  return *this;
}

template <typename T>
typename ForwardSerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(T const &val)
{
  try
  {
    ForwardSerializer<T, SizeCounter>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename ForwardSerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T &val)
{
  try
  {
    ForwardSerializer<T, SizeCounter>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename IntegerSerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(T const &val)
{
  try
  {
    IntegerSerializer<T, SizeCounter>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename IntegerSerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T &val)
{
  try
  {
    IntegerSerializer<T, SizeCounter>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename FloatSerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(T const &val)
{
  try
  {
    FloatSerializer<T, SizeCounter>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename FloatSerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T &val)
{
  try
  {
    FloatSerializer<T, SizeCounter>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BooleanSerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(T const &val)
{
  try
  {
    BooleanSerializer<T, SizeCounter>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename BooleanSerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T &val)
{
  try
  {
    BooleanSerializer<T, SizeCounter>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename StringSerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(T const &val)
{
  try
  {
    StringSerializer<T, SizeCounter>::Serialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error serializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

template <typename T>
typename StringSerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T &val)
{
  try
  {
    StringSerializer<T, SizeCounter>::Deserialize(*this, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }
  return *this;
}

template <typename T>
typename BinarySerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(T const &val)
{
  using Serializer = BinarySerializer<T, SizeCounter>;
  using Constructor =
      interfaces::BinaryConstructorInterface<SizeCounter, TypeCodes::BINARY_CODE8,
                                             TypeCodes::BINARY_CODE16, TypeCodes::BINARY_CODE32>;

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
typename BinarySerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T &val)
{
  using Serializer = BinarySerializer<T, SizeCounter>;
  try
  {
    interfaces::BinaryDeserializer<SizeCounter> stream(*this);
    Serializer::Deserialize(stream, val);
  }
  catch (std::exception const &e)
  {
    throw std::runtime_error("Error deserializing " + static_cast<std::string>(typeid(T).name()) +
                             ".\n" + std::string(e.what()));
  }

  return *this;
}

inline SizeCounter::ArrayConstructor SizeCounter::NewArrayConstructor()
{
  return ArrayConstructor(*this);
}

template <typename T>
typename ArraySerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(T const &val)
{
  using Serializer = ArraySerializer<T, SizeCounter>;
  try
  {
    ArrayConstructor constructor(*this);
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
typename ArraySerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T &val)
{
  using Serializer        = ArraySerializer<T, SizeCounter>;
  using ArrayDeserializer = interfaces::ArrayDeserializer<SizeCounter>;

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

inline SizeCounter::MapConstructor SizeCounter::NewMapConstructor()
{
  return MapConstructor(*this);
}

template <typename T>
typename MapSerializer<T, SizeCounter>::DriverType &SizeCounter::operator<<(T const &val)
{
  using Serializer = MapSerializer<T, SizeCounter>;

  try
  {
    MapConstructor constructor(*this);
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
typename MapSerializer<T, SizeCounter>::DriverType &SizeCounter::operator>>(T &val)
{
  using Serializer      = MapSerializer<T, SizeCounter>;
  using MapDeserializer = interfaces::MapDeserializer<SizeCounter>;

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
auto sizeCounterGuardFactory(T &size_counter);

/**
 * @brief Guard for size count algorithm used in the recursive Append(...args) methods
 * of STREAM/BUFFER like classes.
 *
 * This class is used inside of recursive variadic-based algorithm of Append(...args)
 * methods to make sure that stream/buffer size counting is started only ONCE in the
 * whole recursivecall process AND is properly finished at the end of it (when recursive
 * size counting is reset back zero).
 * This guard is implemented as class to ensure correct functionality in exception based
 * environment.
 *
 * @tparam T Represents type with STREAM/BUFFER like API (e.g. SizeCounter, MsgPackSerializer,
 * MsgPackSerializer, etc. ...), with clear preference to use here size count
 * implementation (SizeCounter class) due to performance reasons.
 */
template <typename T>
class SizeCounterGuard
{
public:
  using SizeCounterType = T;

private:
  friend auto sizeCounterGuardFactory<T>(T &size_counter);

  T *size_counter_;

  explicit SizeCounterGuard(T *size_counter)
    : size_counter_{size_counter}
  {}

public:
  SizeCounterGuard(SizeCounterGuard &&) noexcept = default;

  /**
   * @brief Destructor ensures that size counting instance is reset to zero at the end
   * of recursive Append(..args) process.
   *
   * The reseting to zero makes sure that next call to Append(...arg) recursive
   * method will start from zero size count.
   */
  ~SizeCounterGuard()
  {
    if (size_counter_)
    {
      // Resetting size counter to zero size by reconstructing it
      *size_counter_ = SizeCounterType{};
    }
  }

  SizeCounterGuard(SizeCounterGuard const &) = delete;
  SizeCounterGuard &operator=(SizeCounterGuard const &) = delete;
  SizeCounterGuard &operator=(SizeCounterGuard &&) = delete;

  /**
   * @brief Indicates whether we are already in size counting process
   *
   * This method is inteded to be used in recursive call enviromnet of Append(...args)
   * variadic methods to detect whether size counted process is in progress, and so
   * then ultimatelly it is supposed to be used to protect recursive code against starting
   * the counting process again.
   *
   * @return true if size counting process did not start yet
   * @return false if size counting process is in progress
   */
  bool is_unreserved() const
  {
    return size_counter_ && size_counter_->size() == 0;
  }
};

template <typename T>
auto sizeCounterGuardFactory(T &size_counter)
{
  return SizeCounterGuard<T>{size_counter.size() == 0 ? &size_counter : nullptr};
}

}  // namespace serializers
}  // namespace fetch
