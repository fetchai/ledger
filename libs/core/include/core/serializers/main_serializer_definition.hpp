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

class MsgPackByteArrayBuffer
{
  static char const *const LOGGING_NAME;

public:
  using byte_array_type   = byte_array::ByteArray;
  using size_counter_type = serializers::SizeCounter<MsgPackByteArrayBuffer>;


  MsgPackByteArrayBuffer()                              = default;
  MsgPackByteArrayBuffer(MsgPackByteArrayBuffer &&from) = default;
  MsgPackByteArrayBuffer &operator=(MsgPackByteArrayBuffer &&from) = default;


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
  MsgPackByteArrayBuffer(byte_array::ByteArray const &s);

  // TODO: We should implement move constructor to allow maximal efficiency

  MsgPackByteArrayBuffer(MsgPackByteArrayBuffer const &from);

  MsgPackByteArrayBuffer &operator=(MsgPackByteArrayBuffer const &from);

  void Allocate(uint64_t const &delta);

  void Resize(uint64_t const &   size,
              ResizeParadigm const &resize_paradigm     = ResizeParadigm::RELATIVE,
              bool const            zero_reserved_space = true);

  void Reserve(uint64_t const &   size,
               ResizeParadigm const &resize_paradigm     = ResizeParadigm::RELATIVE,
               bool const            zero_reserved_space = true);
  void WriteBytes(uint8_t const *arr, uint64_t const &size);

  void WriteByte(uint8_t const &val);

  template< typename WriteType, typename InitialType>
  void WritePrimitive(InitialType const &val);

  template< typename ReadType, typename FinalType>
  void ReadPrimitive(FinalType &val);
  
  void ReadByte(uint8_t &val);

  void ReadBytes(uint8_t *arr, uint64_t const &size);

  void ReadByteArray(byte_array::ConstByteArray &b, uint64_t const &size);
  void SkipBytes(uint64_t const &size);

  template <typename T>
  typename IgnoredSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &);

  template <typename T>
  typename IgnoredSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &);

  template <typename T>
  typename ForwardSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename ForwardSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &val);

  template <typename T>
  typename IntegerSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename IntegerSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &val);

  template <typename T>
  typename FloatSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename FloatSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &val);

  template <typename T>
  typename BooleanSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename BooleanSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &val);


  template <typename T>
  typename StringSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename StringSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &val);

  template <typename T>
  typename BinarySerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename BinarySerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &val);

  template <typename T>
  typename ArraySerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &val);


  template <typename T>
  typename ArraySerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &val);
  template <typename T>
  typename MapSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator<<(T const &val);

  template <typename T>
  typename MapSerializer<T, MsgPackByteArrayBuffer>::DriverType &operator>>(T &val);

  template <typename T>
  MsgPackByteArrayBuffer &Pack(T const *val);

  template <typename T>
  MsgPackByteArrayBuffer &Pack(T const &val);

  template <typename T>
  MsgPackByteArrayBuffer &Unpack(T &val);

  void seek(uint64_t p);
  uint64_t tell() const;

  uint64_t size() const;
  uint64_t capacity() const;

  int64_t bytes_left() const;
  byte_array::ByteArray const &data() const;

  template <typename... ARGS>
  MsgPackByteArrayBuffer &Append(ARGS const &... args);

private:
  template <typename T, typename... ARGS>
  void AppendInternal(T const &arg, ARGS const &... args);
  void AppendInternal();

  byte_array_type   data_;
  uint64_t          pos_ = 0;
  size_counter_type size_counter_;
};



}  // namespace serializers
}  // namespace fetch
