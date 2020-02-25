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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/realign.hpp"
#include "meta/type_traits.hpp"
#include "meta/value_util.hpp"

#include <cstring>
#include <numeric>
#include <string>
#include <utility>

namespace fetch {
namespace buffer_io {

namespace detail_ {

struct Yes
{
};
struct No
{
};

class BinaryTraits
{
  template <class T>
  static constexpr auto HasSize(T &&t) noexcept -> decltype(t.BinarySize(), Yes{});
  static constexpr No   HasSize(...) noexcept;

  template <class T>
  static constexpr auto HasRead(T &&t) noexcept -> decltype(t.BinaryRead(nullptr), Yes{});
  static constexpr No   HasRead(...) noexcept;

  template <class T>
  static constexpr auto HasWrite(T &&t) noexcept -> decltype(t.BinaryWrite(nullptr), Yes{});
  static constexpr No   HasWrite(...) noexcept;

public:
  template <class T>
  static constexpr bool HasBinarySize =
      std::is_same<decltype(HasSize(std::declval<T>())), Yes>::value;
  template <class T>
  static constexpr bool HasBinaryRead =
      std::is_same<decltype(HasRead(std::declval<T>())), Yes>::value;
  template <class T>
  static constexpr bool HasBinaryWrite =
      std::is_same<decltype(HasWrite(std::declval<T>())), Yes>::value;
};

template <class T>
static constexpr bool IsSizePOD = meta::IsPOD<std::decay_t<T>> && !BinaryTraits::HasBinarySize<T>;

template <class T, class U = void>
using IfIsSizePOD = meta::EnableIf<IsSizePOD<T>, U>;

template <class T>
static constexpr bool IsReadPOD = meta::IsPOD<std::decay_t<T>> && !BinaryTraits::HasBinaryRead<T>;

template <class T, class U = void>
using IfIsReadPOD = meta::EnableIf<IsReadPOD<T>, U>;

template <class T>
static constexpr bool IsWritePOD = meta::IsPOD<std::decay_t<T>> && !BinaryTraits::HasBinaryWrite<T>;

template <class T, class U = void>
using IfIsWritePOD = meta::EnableIf<IsWritePOD<T>, U>;

struct BinarySize
{
  template <class T>
  constexpr auto operator()(std::size_t accum, T &&t) const noexcept
      -> decltype(accum + t.BinarySize())
  {
    return accum + t.BinarySize();
  }

  template <class T>
  constexpr IfIsSizePOD<T, std::size_t> operator()(std::size_t accum, T /*t*/) const noexcept
  {
    return accum + sizeof(T);
  }
};

struct BufferIStream
{
  template <class T>
  constexpr auto operator()(char const *buffer, T &t) const
      -> decltype(std::forward<T>(t).BinaryRead(buffer))
  {
    auto ret_val = std::forward<T>(t).BinaryRead(buffer);
    assert(ret_val == buffer + BinarySize{}(0, t));
    return ret_val;
  }

  template <class T>
  constexpr IfIsReadPOD<T, char const *> operator()(char const *buffer, T &t) const
  {
    std::memcpy(&t, buffer, sizeof(T));
    return buffer + sizeof(T);
  }
};

struct BufferOStream
{
  template <class T>
  constexpr auto operator()(char *buffer, T &&t) const
      -> decltype(std::forward<T>(t).BinaryWrite(buffer))
  {
    auto ret_val = std::forward<T>(t).BinaryWrite(buffer);
    assert(ret_val == buffer + BinarySize{}(0, t));
    return ret_val;
  }

  template <class T>
  constexpr IfIsWritePOD<T, char *> operator()(char *buffer, T t) const
  {
    std::memcpy(buffer, &t, sizeof(T));
    return buffer + sizeof(T);
  }
};

template <class T, class... Ts>
using AreSameSizePOD = std::integral_constant<bool, IsSizePOD<T> && type_util::AreSameV<T, Ts...>>;
template <class T, class... Ts>
static constexpr auto AreSameSizePODV = AreSameSizePOD<T, Ts...>::value;

template <class T, class... Ts>
using AreSameReadPOD = std::integral_constant<bool, IsReadPOD<T> && type_util::AreSameV<T, Ts...>>;
template <class T, class... Ts>
static constexpr auto AreSameReadPODV = AreSameReadPOD<T, Ts...>::value;

template <class T, class... Ts>
using AreSameWritePOD =
    std::integral_constant<bool, IsWritePOD<T> && type_util::AreSameV<T, Ts...>>;
template <class T, class... Ts>
static constexpr auto AreSameWritePODV = AreSameWritePOD<T, Ts...>::value;

}  // namespace detail_

constexpr char const *BufRead(char const *buf) noexcept
{
  return buf;
}

template <class T, class... Ts>
constexpr std::enable_if_t<detail_::AreSameReadPODV<T, Ts...>, char const *> BufRead(
    char const *buf, T t, Ts... ts)
{
  constexpr std::size_t amount        = 1 + sizeof...(Ts);
  auto const            realigned_buf = memory::Realign<T>(buf, amount);

  value_util::Accumulate(
      [](T const *buf, T &&var) {
        var = *buf;
        return buf + 1;
      },
      realigned_buf, std::forward<T>(t), std::forward<Ts>(ts)...);

  return buf + amount * sizeof(T);
}

template <class T, class... Ts>
constexpr std::enable_if_t<!detail_::AreSameReadPODV<T, Ts...>, char const *> BufRead(
    char const *buf, T &&t, Ts &&... ts)
{
  return value_util::Accumulate(detail_::BufferIStream{}, buf, std::forward<T>(t),
                                std::forward<Ts>(ts)...);
}

template <class Stream>
constexpr std::size_t FRead(Stream &&stream) noexcept
{
  return 0;
}

template <class Stream, class T>
constexpr std::enable_if_t<detail_::IsReadPOD<T>, std::size_t> FRead(Stream &&stream, T &&t)
{
  stream.read(reinterpret_cast<char *>(&t), sizeof(T));
  return sizeof(T);
}

template <class Stream, class T, class... Ts>
constexpr std::enable_if_t<detail_::AreSameReadPODV<T, Ts...>, std::size_t> FRead(Stream &&stream,
                                                                                  T &&     t,
                                                                                  Ts &&... ts)
{
  T buf[sizeof...(Ts) + 1];
  stream.read(&buf, sizeof(buf));

  value_util::Accumulate(
      [](T *buf, T &&var) {
        var = *buf;
        return buf + 1;
      },
      static_cast<T const *>(buf), std::forward<T>(t), std::forward<Ts>(ts)...);

  return sizeof(buf);
}

template <class Stream, class T, class... Ts>
constexpr std::enable_if_t<!detail_::AreSameReadPODV<T, Ts...>, std::size_t> FRead(Stream &&stream,
                                                                                   T &&     t,
                                                                                   Ts &&... ts)
{
  const std::size_t size = value_util::Accumulate(detail_::BinarySize{}, 0, ts...);

  byte_array::ByteArray buf{byte_array::ConstByteArray(size)};
  stream.read(buf.char_pointer(), size);

  value_util::Accumulate(detail_::BufferIStream{}, buf.char_pointer(), std::forward<T>(t),
                         std::forward<Ts>(ts)...);
  return size;
}

template <class Stream, class T>
constexpr std::enable_if_t<detail_::IsReadPOD<T>, std::size_t> BulkRead(Stream &&stream, T *t,
                                                                        std::size_t amount)
{
  constexpr std::size_t size = sizeof(T) * amount;
  stream.read(t, size);
  return size;
}

template <class Stream, class T>
constexpr meta::EnableIf<!detail_::IsReadPOD<T>, std::size_t> BulkRead(Stream &&stream, T *t,
                                                                       std::size_t amount)
{
  if (amount == 0)
  {
    return 0;
  }

  detail_::BinarySize sz_op;
  auto const          object_size = sz_op(0, *t);
  for (std::size_t i = 1; i < amount; ++i)
  {
    assert(sz_op(0, t[i]) == object_size);
  }

  auto const            size = object_size * amount;
  byte_array::ByteArray buf{byte_array::ConstByteArray(size)};
  char *                buf_stream = buf.char_pointer();
  stream.read(buf_stream, size);

  std::accumulate(t, t + amount, const_cast<char const *>(buf_stream), detail_::BufferIStream{});
  return size;
}

template <class T, class... Ts>
constexpr std::enable_if_t<detail_::AreSameWritePODV<T, Ts...>, char *> BufWrite(char *buf, T t,
                                                                                 Ts... ts)
{
  T const array[] = {t, ts...};
  std::memcpy(buf, array, sizeof(array));
  return buf + sizeof(array);
}

template <class T, class... Ts>
constexpr std::enable_if_t<!detail_::AreSameWritePODV<T, Ts...>, char *> BufWrite(char *buf, T &&t,
                                                                                  Ts &&... ts)
{
  return value_util::Accumulate(detail_::BufferOStream{}, buf, std::forward<T>(t),
                                std::forward<Ts>(ts)...);
}

template <class Stream>
constexpr std::size_t FWrite(Stream &&stream) noexcept
{
  return 0;
}

template <class Stream, class T>
constexpr std::enable_if_t<detail_::IsWritePOD<T>, std::size_t> FWrite(Stream &&stream, T &&t)
{
  stream.write(reinterpret_cast<char const *>(&t), sizeof(T));
  return sizeof(T);
}

template <class Stream, class T, class... Ts>
constexpr std::enable_if_t<detail_::AreSameWritePODV<T, Ts...>, std::size_t> FWrite(Stream &&stream,
                                                                                    T t, Ts... ts)
{
  T buf[sizeof...(Ts) + 1] = {t, ts...};
  stream.write(buf, sizeof(buf));
  return sizeof(buf);
}

template <class Stream, class T, class... Ts>
constexpr std::enable_if_t<!detail_::AreSameWritePODV<T, Ts...>, std::size_t> FWrite(
    Stream &&stream, T &&t, Ts &&... ts)
{
  const std::size_t size = value_util::Accumulate(detail_::BinarySize{}, 0, ts...);

  byte_array::ByteArray buf{byte_array::ConstByteArray(size)};
  value_util::Accumulate(detail_::BufferOStream{}, buf.char_pointer(), std::forward<T>(t),
                         std::forward<Ts>(ts)...);

  stream.write(buf.char_pointer(), size);
  return size;
}

template <class Stream, class T>
constexpr std::enable_if_t<meta::IsPOD<T>, std::size_t> BulkWrite(Stream &&stream, T *t,
                                                                  std::size_t amount)
{
  constexpr std::size_t size = sizeof(T) * amount;
  stream.write(t, size);
  return size;
}

template <class Stream, class T>
constexpr meta::EnableIf<!detail_::IsWritePOD<T>, std::size_t> BulkWrite(Stream &&   stream,
                                                                         T const *   t,
                                                                         std::size_t amount)
{
  if (amount == 0)
  {
    return 0;
  }

  detail_::BinarySize sz_op;
  auto const          object_size = sz_op(0, *t);
  for (std::size_t i = 1; i < amount; ++i)
  {
    assert(sz_op(0, t[i]) == object_size);
  }

  auto const            size = object_size * amount;
  byte_array::ByteArray buffer{byte_array::ConstByteArray(size)};
  char *                buf = buffer.char_pointer();
  std::accumulate(t, t + amount, buf, detail_::BufferOStream{});

  stream.write(buf, size);
  return size;
}

// TODO (nobody): the users of these functions should be refactored to not use them
template <class T>
char const *DirtyBufRead(char const *buf, T &&t)
{
  std::memcpy(static_cast<void *>(&t), buf, sizeof(T));
  return buf + sizeof(t);
}

template <class T>
char *DirtyBufWrite(char *buf, T &&t)
{
  std::memcpy(buf, static_cast<void const *>(&t), sizeof(T));
  return buf + sizeof(t);
}

}  // namespace buffer_io
}  // namespace fetch
