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
#include "core/serializers/counter.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/group_definitions.hpp"
#include "vectorise/platform.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

class MsgPackSerializer
{
public:
  using ByteArray = byte_array::ByteArray;

  /// Array Helpers
  /// @{
  using ArrayConstructor = interfaces::ContainerConstructorInterface<
      MsgPackSerializer, interfaces::ArrayInterface<MsgPackSerializer>, TypeCodes::ARRAY_CODE_FIXED,
      TypeCodes::ARRAY_CODE16, TypeCodes::ARRAY_CODE32>;
  using ArrayDeserializer = interfaces::ArrayDeserializer<MsgPackSerializer>;
  /// @}

  /// Map Helpers
  /// @{
  using MapConstructor = interfaces::ContainerConstructorInterface<
      MsgPackSerializer, interfaces::MapInterface<MsgPackSerializer>, TypeCodes::MAP_CODE_FIXED,
      TypeCodes::MAP_CODE16, TypeCodes::MAP_CODE32>;
  using MapDeserializer = interfaces::MapDeserializer<MsgPackSerializer>;
  /// @}

  /// Pair Helpers
  /// @{
  using PairConstructor = interfaces::ContainerConstructorInterface<
      MsgPackSerializer, interfaces::PairInterface<MsgPackSerializer>, TypeCodes::PAIR_CODE_FIXED,
      TypeCodes::PAIR_CODE16, TypeCodes::PAIR_CODE32>;
  using PairDeserializer = interfaces::PairDeserializer<MsgPackSerializer>;
  /// @}

  MsgPackSerializer()                         = default;
  MsgPackSerializer(MsgPackSerializer &&from) = default;
  MsgPackSerializer &operator=(MsgPackSerializer &&from) = default;

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
  explicit MsgPackSerializer(byte_array::ByteArray s);
  MsgPackSerializer(MsgPackSerializer const &from);

  MsgPackSerializer &operator=(MsgPackSerializer const &from);

  SerializerTypes GetNextType() const
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
  typename IgnoredSerializer<T, MsgPackSerializer>::DriverType &operator<<(T const & /*unused*/);

  template <typename T>
  typename IgnoredSerializer<T, MsgPackSerializer>::DriverType &operator>>(T & /*unused*/);

  template <typename T>
  typename ForwardSerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename ForwardSerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);

  template <typename T>
  typename IntegerSerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename IntegerSerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);

  template <typename T>
  typename FloatSerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename FloatSerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);

  template <typename T>
  typename BooleanSerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename BooleanSerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);

  template <typename T>
  typename StringSerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename StringSerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);

  template <typename T>
  typename BinarySerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename BinarySerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);

  ArrayConstructor  NewArrayConstructor();
  ArrayDeserializer NewArrayDeserializer();

  template <typename T>
  typename ArraySerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename ArraySerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);

  MapConstructor  NewMapConstructor();
  MapDeserializer NewMapDeserializer();

  PairConstructor  NewPairConstructor();
  PairDeserializer NewPairDeserializer();

  template <typename T>
  typename MapSerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename MapSerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);

  template <typename T>
  MsgPackSerializer &Pack(T const *val);

  template <typename T>
  MsgPackSerializer &Pack(T const &val);

  template <typename T>
  MsgPackSerializer &Unpack(T &val);

  void     seek(uint64_t p);
  uint64_t tell() const;

  uint64_t size() const;
  uint64_t capacity() const;

  int64_t                      bytes_left() const;
  byte_array::ByteArray const &data() const;

  template <typename... ARGS>
  MsgPackSerializer &Append(ARGS const &... args);

private:
  template <typename T, typename... ARGS>
  void AppendInternal(T const &arg, ARGS const &... args);
  void AppendInternal();

  ByteArray   data_;
  uint64_t    pos_ = 0;
  SizeCounter size_counter_;
};

}  // namespace serializers
}  // namespace fetch
