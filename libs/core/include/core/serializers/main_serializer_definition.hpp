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
#include "core/serializers/counter.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/group_definitions.hpp"

#include "vectorise/platform.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

class MsgPackSerializer
{
  static char const *const LOGGING_NAME;

public:
  using ByteArray = byte_array::ByteArray;

  MsgPackSerializer()                         = default;
  MsgPackSerializer(MsgPackSerializer &&from) = default;
  MsgPackSerializer &operator=(MsgPackSerializer &&from) = default;

  /**
   * @brief Contructting from MUTABLE ByteArray.
   *
   * DEEP copy is made here due to safety reasons to avoid later
   * misshaps & missunderstrandings related to what hapens with reserved
   * memory of mutable @ref s instance passed in by caller of this
   * constructor once this class starts to modify content of underlaying
   * internal @ref data_ ByteArray and then resize/reserve it.
   *
   * @param s Input mutable instance of ByteArray to copy content from (by
   *          value as explained above)
   */
  MsgPackSerializer(byte_array::ByteArray const &s);
  MsgPackSerializer(MsgPackSerializer const &from);

  MsgPackSerializer &operator=(MsgPackSerializer const &from);

  void Allocate(uint64_t const &delta);

  void Resize(uint64_t const &      size,
              ResizeParadigm const &resize_paradigm     = ResizeParadigm::RELATIVE,
              bool const            zero_reserved_space = true);

  void Reserve(uint64_t const &      size,
               ResizeParadigm const &resize_paradigm     = ResizeParadigm::RELATIVE,
               bool const            zero_reserved_space = true);
  void WriteBytes(uint8_t const *arr, uint64_t const &size);

  void WriteByte(uint8_t const &val);

  template <typename WriteType, typename InitialType>
  void WritePrimitive(InitialType const &val);

  template <typename ReadType, typename FinalType>
  void ReadPrimitive(FinalType &val);

  void ReadByte(uint8_t &val);

  void ReadBytes(uint8_t *arr, uint64_t const &size);

  void ReadByteArray(byte_array::ConstByteArray &b, uint64_t const &size);
  void SkipBytes(uint64_t const &size);

  template <typename T>
  typename IgnoredSerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &);

  template <typename T>
  typename IgnoredSerializer<T, MsgPackSerializer>::DriverType &operator>>(T &);

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

  template <typename T>
  typename ArraySerializer<T, MsgPackSerializer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename ArraySerializer<T, MsgPackSerializer>::DriverType &operator>>(T &val);
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
