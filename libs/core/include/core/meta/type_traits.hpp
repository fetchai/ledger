#pragma once

#include "core/byte_array/byte_array.hpp"

#include <type_traits>

namespace fetch {
namespace meta {

template <bool C, typename R = void>
using EnableIf = typename std::enable_if<C, R>::type;

template <typename T, typename R = T>
using IfIsIntegerLike = EnableIf<(!std::is_same<T, bool>::value) && std::is_integral<T>::value, R>;

template <typename T, typename R = T>
using IfIsFloatLike = EnableIf<std::is_floating_point<T>::value, R>;

template <typename T, typename R = T>
using IfIsBooleanLike = EnableIf<std::is_same<T, bool>::value, R>;

template <typename T>
constexpr bool IsByteArrayLike = (std::is_same<T, byte_array::ByteArray>::value ||
                                  std::is_same<T, byte_array::ConstByteArray>::value);

template <typename T, typename R = T>
using IfIsByteArrayLike = EnableIf<IsByteArrayLike<T>, R>;

template <typename T, typename R = T>
using IfIsStdStringLike = EnableIf<std::is_same<T, std::string>::value, R>;

}  // namespace meta
}  // namespace fetch
