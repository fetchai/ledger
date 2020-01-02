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

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace fetch {
namespace memory {

template <typename T>
class ForwardIterator : public std::iterator<std::forward_iterator_tag, T>
{
public:
  ForwardIterator() = delete;

  constexpr ForwardIterator(ForwardIterator const &other)     = default;
  constexpr ForwardIterator(ForwardIterator &&other) noexcept = default;
  constexpr ForwardIterator &operator=(ForwardIterator const &other) = default;
  constexpr ForwardIterator &operator=(ForwardIterator &&other) noexcept = default;

  constexpr explicit ForwardIterator(T *pos) noexcept
    : pos_(pos)
  {}
  constexpr ForwardIterator(T *pos, T *end) noexcept
    : pos_(pos)
    , end_(end)
  {}

  constexpr ForwardIterator &operator++() noexcept
  {
    ++pos_;
    return *this;
  }

  constexpr T &operator*() noexcept
  {
    assert(pos_ != nullptr);
    assert((end_ != nullptr) && (pos_ < end_));
    return *pos_;
  }

  constexpr bool operator==(ForwardIterator const &other) const noexcept
  {
    return other.pos_ == pos_;
  }
  constexpr bool operator!=(ForwardIterator const &other) const noexcept
  {
    return other.pos_ != pos_;
  }

private:
  T *pos_ = nullptr;
  T *end_ = nullptr;
};

template <typename T>
class BackwardIterator : public std::iterator<std::forward_iterator_tag, T>
{
public:
  constexpr BackwardIterator() = delete;

  constexpr BackwardIterator(BackwardIterator const &other)     = default;
  constexpr BackwardIterator(BackwardIterator &&other) noexcept = default;
  constexpr BackwardIterator &operator=(BackwardIterator const &other) = default;
  constexpr BackwardIterator &operator=(BackwardIterator &&other) noexcept = default;

  constexpr explicit BackwardIterator(T *pos) noexcept
    : pos_(pos)
  {}
  constexpr BackwardIterator(T *pos, T *begin) noexcept
    : pos_(pos)
    , begin_(begin)
  {}

  constexpr BackwardIterator &operator++() noexcept
  {
    --pos_;
    return *this;
  }

  constexpr T &operator*() noexcept
  {
    assert(pos_ != nullptr);
    assert((begin_ != nullptr) && (pos_ > begin_));
    return *pos_;
  }

  constexpr bool operator==(BackwardIterator const &other) const noexcept
  {
    return other.pos_ == pos_;
  }
  constexpr bool operator!=(BackwardIterator const &other) const noexcept
  {
    return other.pos_ != pos_;
  }

private:
  T *pos_   = nullptr;
  T *begin_ = nullptr;
};

}  // namespace memory
}  // namespace fetch

namespace std {

template <typename T>
struct iterator_traits<fetch::memory::ForwardIterator<T>>  // NOLINT
{
  using difference_type   = std::ptrdiff_t;             // NOLINT
  using value_type        = std::remove_cv_t<T>;        // NOLINT
  using pointer           = T *;                        // NOLINT
  using reference         = T &;                        // NOLINT
  using iterator_category = std::forward_iterator_tag;  // NOLINT
};

template <typename T>
struct iterator_traits<fetch::memory::BackwardIterator<T>>  // NOLINT
{
  using difference_type   = std::ptrdiff_t;             // NOLINT
  using value_type        = std::remove_cv_t<T>;        // NOLINT
  using pointer           = T *;                        // NOLINT
  using reference         = T &;                        // NOLINT
  using iterator_category = std::forward_iterator_tag;  // NOLINT
};

}  // namespace std
