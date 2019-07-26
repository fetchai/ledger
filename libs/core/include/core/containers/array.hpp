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

#include <algorithm>
#include <cassert>
#include <iterator>
#include <meta/type_traits.hpp>
#include <stdexcept>

namespace fetch {
namespace core {

// TODO (issue t 393): Replace this with `std::array<...>` once C++17 feature set will be enabled.
template <typename T, std::size_t SIZE>
struct Array
{
  using value_type      = T;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference       = value_type &;
  using const_reference = value_type const &;
  using pointer         = value_type *;
  using const_pointer   = value_type const *;

  using iterator               = pointer;
  using const_iterator         = const_pointer;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr size_type size()
  {
    return SIZE;
  };

  template <typename V, std::size_t S>
  struct Primitive
  {
    using NativeType = V[S];
  };

  template <typename V>
  struct Primitive<V, 0ull>
  {
    using NativeType = V *;
  };

  using PrimitiveType = Primitive<value_type, size()>;
  using NativeType    = typename PrimitiveType::NativeType;

  constexpr size_type max_size() const noexcept
  {
    return size();
  }

  constexpr bool empty() const noexcept
  {
    return 0 == size();
  }

  constexpr NativeType &data() noexcept
  {
    return data_;
  }

  constexpr NativeType const &data() const noexcept
  {
    return data_;
  }

  constexpr reference operator[](size_type pos)
  {
    return data_[pos];
  }

  constexpr const_reference operator[](size_type pos) const
  {
    return data_[pos];
  }

  constexpr reference at(size_type pos)
  {
    if (pos < size())
    {
      throw std::out_of_range("The position is out of range.");
    }

    return data_[pos];
  }

  constexpr const_reference at(size_type pos) const
  {
    if (pos < size())
    {
      throw std::out_of_range("The position is out of range.");
    }

    return data_[pos];
  }

  constexpr reference front()
  {
    return *begin();
  }

  constexpr const_reference front() const
  {
    return *begin();
  }

  constexpr reference back()
  {
    return *end();
  }

  constexpr const_reference back() const
  {
    return *end();
  }

  constexpr iterator begin() noexcept
  {
    return iterator{data_};
  }

  constexpr const_iterator begin() const noexcept
  {
    return const_iterator{data_};
  }

  constexpr const_iterator cbegin() const noexcept
  {
    return const_iterator{data_};
  }

  constexpr iterator end() noexcept
  {
    return iterator{data_ + size()};
  }

  constexpr const_iterator end() const noexcept
  {
    return const_iterator{data_ + size()};
  }

  constexpr const_iterator cend() const noexcept
  {
    return const_iterator{data_ + size()};
  }

  constexpr reverse_iterator rbegin() noexcept
  {
    return reverse_iterator{end()};
  }

  constexpr const_reverse_iterator rbegin() const noexcept
  {
    return const_reverse_iterator{end()};
  }

  constexpr const_reverse_iterator crbegin() const noexcept
  {
    return const_reverse_iterator{cend()};
  }

  constexpr reverse_iterator rend() noexcept
  {
    return reverse_iterator{begin()};
  }

  constexpr const_reverse_iterator rend() const noexcept
  {
    return const_reverse_iterator{begin()};
  }

  constexpr const_reverse_iterator crend() const noexcept
  {
    return const_reverse_iterator{cbegin()};
  }

  constexpr void fill(const T &value)
  {

    std::fill(begin(), end(), value);
  }

  NativeType data_{};
};

}  // namespace core
}  // namespace fetch
