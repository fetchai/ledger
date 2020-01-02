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

#include "vectorise/info.hpp"
#include "vectorise/meta/math_type_traits.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ostream>

namespace fetch {

namespace vectorise {

template <typename T>
struct VectorRegisterSize
{
  enum
  {
    value = 64
  };
};

#define ADD_REGISTER_SIZE(type, size) \
  template <>                         \
  struct VectorRegisterSize<type>     \
  {                                   \
    enum                              \
    {                                 \
      value = (size)                  \
    };                                \
  }

#include <cmath>
#include <cstddef>
#include <ostream>

namespace details {
template <typename T, std::size_t N>
struct UnrollSet
{
  static void Set(T *ptr, T const &c)
  {
    (*ptr) = c;
    UnrollSet<T, N - 1>::Set(ptr + 1, c);
  }
};

template <typename T>
struct UnrollSet<T, 0>
{
  static void Set(T * /*ptr*/, T const & /*c*/)
  {}
};
}  // namespace details

// clang-format off
// NOLINTNEXTLINE
#define APPLY_OPERATOR_LIST(FUNCTION) \
  FUNCTION(*)                         \
  FUNCTION(/)                         \
  FUNCTION(+)                         \
  FUNCTION(-)                         \
  FUNCTION(&)                         \
  FUNCTION(|)                         \
  FUNCTION(^)
// clang-format on

template <typename T, std::size_t N = 8 * sizeof(T)>
class VectorRegister : public BaseVectorRegisterType
{
public:
  using type           = T;
  using MMRegisterType = T;

  enum
  {
    E_VECTOR_SIZE   = sizeof(MMRegisterType),
    E_REGISTER_SIZE = sizeof(MMRegisterType),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() = default;
  explicit VectorRegister(type const *d)
    : data_(*d)
  {}
  explicit VectorRegister(type const &d)
    : data_(d)
  {}
  explicit VectorRegister(type &&d)
    : data_(d)
  {}

  explicit operator T()
  {
    return data_;
  }

  type data() const
  {
    return data_;
  }
  type &data()
  {
    return data_;
  }

#define FETCH_ADD_OPERATOR(OP)                                  \
  VectorRegister operator OP(VectorRegister const &other) const \
  {                                                             \
    return VectorRegister(type(data_ OP other.data_));          \
  }
  APPLY_OPERATOR_LIST(FETCH_ADD_OPERATOR);  // NOLINT
#undef FETCH_ADD_OPERATOR

  void Store(type *ptr) const
  {
    *ptr = data_;
  }

private:
  type data_;
};

template <typename T, std::size_t N = 8 * sizeof(T)>
inline std::ostream &operator<<(std::ostream &s, VectorRegister<T, N> const &n)
{
  s << n.data();
  return s;
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline VectorRegister<T, N> abs(VectorRegister<T, N> const &x)
{
  return VectorRegister<T, N>(std::abs(x.data()));
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline VectorRegister<T, N> approx_log(VectorRegister<T, N> const &x)
{
  return VectorRegister<T, N>(std::log(x.data()));
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline VectorRegister<T, N> approx_exp(VectorRegister<T, N> const &x)
{
  return VectorRegister<T, N>(std::exp(x.data()));
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline VectorRegister<T, N> shift_elements_right(VectorRegister<T, N> const &x)
{
  return VectorRegister<T, N>(x.data());
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline VectorRegister<T, N> shift_elements_left(VectorRegister<T, N> const &x)
{
  return VectorRegister<T, N>(x.data());
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline T first_element(VectorRegister<T, N> const &x)
{
  return x.data();
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline T reduce(VectorRegister<T, N> const &x)
{
  return x.data();
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline bool all_less_than(VectorRegister<T, N> const &x, VectorRegister<T, N> const &y)
{
  return x.data() < y.data();
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline bool any_less_than(VectorRegister<T, N> const &x, VectorRegister<T, N> const &y)
{
  return x.data() < y.data();
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline bool all_equal_to(VectorRegister<T, N> const &x, VectorRegister<T, N> const &y)
{
  return x.data() == y.data();
}

template <typename T, std::size_t N = 8 * sizeof(T)>
inline bool any_equal_to(VectorRegister<T, N> const &x, VectorRegister<T, N> const &y)
{
  return x.data() == y.data();
}

#undef APPLY_OPERATOR_LIST
}  // namespace vectorise
}  // namespace fetch
