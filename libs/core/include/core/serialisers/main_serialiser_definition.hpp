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
#include "core/serialisers/counter.hpp"
#include "core/serialisers/exception.hpp"
#include "core/serialisers/group_definitions.hpp"
#include "vectorise/platform.hpp"

#include <type_traits>

namespace fetch {
namespace serialisers {

class MsgPackSerialiser
{
public:
  using ByteArray = byte_array::ByteArray;

  /// Array Helpers
  /// @{
  using ArrayConstructor = interfaces::ContainerConstructorInterface<
      MsgPackSerialiser, interfaces::ArrayInterface<MsgPackSerialiser>, TypeCodes::ARRAY_CODE_FIXED,
      TypeCodes::ARRAY_CODE16, TypeCodes::ARRAY_CODE32>;
  using ArrayDeserialiser = interfaces::ArrayDeserialiser<MsgPackSerialiser>;
  /// @}

  /// Map Helpers
  /// @{
  using MapConstructor = interfaces::ContainerConstructorInterface<
      MsgPackSerialiser, interfaces::MapInterface<MsgPackSerialiser>, TypeCodes::MAP_CODE_FIXED,
      TypeCodes::MAP_CODE16, TypeCodes::MAP_CODE32>;
  using MapDeserialiser = interfaces::MapDeserialiser<MsgPackSerialiser>;
  /// @}

  /// Pair Helpers
  /// @{
  using PairConstructor = interfaces::ContainerConstructorInterface<
      MsgPackSerialiser, interfaces::PairInterface<MsgPackSerialiser>, TypeCodes::PAIR_CODE_FIXED,
      TypeCodes::PAIR_CODE16, TypeCodes::PAIR_CODE32>;
  using PairDeserialiser = interfaces::PairDeserialiser<MsgPackSerialiser>;
  /// @}

  MsgPackSerialiser()                         = default;
  MsgPackSerialiser(MsgPackSerialiser &&from) = default;
  MsgPackSerialiser &operator=(MsgPackSerialiser &&from) = default;

  /**
   * @brief Constructing from MUTABLE ByteArray.
   *
   * DEEP copy is made here due to safety reasons to avoid later
   * misshaps & misunderstandings related to what happens with reserved
   * memory of mutable @ref s instance passed in by caller of this
   * constructor once this class starts to modify content of underlying
   * internal @ref data_ ByteArray and then resize/reserve it.
   *
   * @param s Input mutable instance of ByteArray to copy content from (by
   *          value as explained above)
   */
  explicit MsgPackSerialiser(byte_array::ByteArray s);
  MsgPackSerialiser(MsgPackSerialiser const &from);

  MsgPackSerialiser &operator=(MsgPackSerialiser const &from);

  SerialiserTypes GetNextType() const
  {
    if (pos_ >= data_.size())
    {
      throw std::runtime_error{"Reached end of the buffer"};
    }

    return DetermineType(data_[pos_]);
  }

  void Allocate(uint64_t const &delta);

  void Resize(uint64_t const &      size,
              ResizeParadigm const &resize_paradigm     = ResizeParadigm::RELATIVE,
              bool                  zero_reserved_space = true);

  void Reserve(uint64_t const &      size,
               ResizeParadigm const &resize_paradigm     = ResizeParadigm::RELATIVE,
               bool                  zero_reserved_space = true);
  void WriteBytes(uint8_t const *arr, uint64_t const &size);

  void WriteByte(uint8_t const &val);
  void WriteNil();

  template <typename WriteType, typename InitialType>
  void WritePrimitive(InitialType const &val);

  template <typename ReadType, typename FinalType>
  void ReadPrimitive(FinalType &val);

  void ReadByte(uint8_t &val);

  void ReadBytes(uint8_t *arr, uint64_t const &size);

  void ReadByteArray(byte_array::ConstByteArray &b, uint64_t const &size);
  void SkipBytes(uint64_t const &size);

  template <typename T>
  typename IgnoredSerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const & /*unused*/);

  template <typename T>
  typename IgnoredSerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T & /*unused*/);

  template <typename T>
  typename ForwardSerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const &val);

  template <typename T>
  typename ForwardSerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T &val);

  template <typename T>
  typename IntegerSerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const &val);

  template <typename T>
  typename IntegerSerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T &val);

  template <typename T>
  typename FloatSerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const &val);

  template <typename T>
  typename FloatSerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T &val);

  template <typename T>
  typename BooleanSerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const &val);

  template <typename T>
  typename BooleanSerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T &val);

  template <typename T>
  typename StringSerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const &val);

  template <typename T>
  typename StringSerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T &val);

  template <typename T>
  typename BinarySerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const &val);

  template <typename T>
  typename BinarySerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T &val);

  ArrayConstructor  NewArrayConstructor();
  ArrayDeserialiser NewArrayDeserialiser();

  template <typename T>
  typename ArraySerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const &val);

  template <typename T>
  typename ArraySerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T &val);

  MapConstructor  NewMapConstructor();
  MapDeserialiser NewMapDeserialiser();

  PairConstructor  NewPairConstructor();
  PairDeserialiser NewPairDeserialiser();

  template <typename T>
  typename MapSerialiser<T, MsgPackSerialiser>::DriverType &operator<<(T const &val);

  template <typename T>
  typename MapSerialiser<T, MsgPackSerialiser>::DriverType &operator>>(T &val);

  template <typename T>
  MsgPackSerialiser &Pack(T const *val);

  template <typename T>
  MsgPackSerialiser &Pack(T const &val);

  template <typename T>
  MsgPackSerialiser &Unpack(T &val);

  void     seek(uint64_t p);
  uint64_t tell() const;

  uint64_t size() const;
  uint64_t capacity() const;

  int64_t                      bytes_left() const;
  byte_array::ByteArray const &data() const;

  template <typename... ARGS>
  MsgPackSerialiser &Append(ARGS const &... args);

private:
  template <typename T, typename... ARGS>
  void AppendInternal(T const &arg, ARGS const &... args);
  void AppendInternal();

  ByteArray   data_;
  uint64_t    pos_ = 0;
  SizeCounter size_counter_;
};

}  // namespace serialisers
}  // namespace fetch
