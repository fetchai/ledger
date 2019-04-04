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

#include <cassert>
#include <iterator>

namespace fetch {
namespace memory {

template <typename T>
class ForwardIterator : public std::iterator<std::forward_iterator_tag, T>
{
public:
  ForwardIterator() = delete;

  ForwardIterator(ForwardIterator const &other) = default;
  ForwardIterator(ForwardIterator &&other)      = default;
  ForwardIterator &operator=(ForwardIterator const &other) = default;
  ForwardIterator &operator=(ForwardIterator &&other) = default;

  ForwardIterator(T *pos)
    : pos_(pos)
  {}
  ForwardIterator(T *pos, T *end)
    : pos_(pos)
    , end_(end)
  {}

  ForwardIterator &operator++()
  {
    ++pos_;
    return *this;
  }

  T &operator*()
  {
    assert(pos_ != nullptr);
    assert((end_ != nullptr) && (pos_ < end_));
    return *pos_;
  }

  bool operator==(ForwardIterator const &other) const
  {
    return other.pos_ == pos_;
  }
  bool operator!=(ForwardIterator const &other) const
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
  BackwardIterator() = delete;

  BackwardIterator(BackwardIterator const &other) = default;
  BackwardIterator(BackwardIterator &&other)      = default;
  BackwardIterator &operator=(BackwardIterator const &other) = default;
  BackwardIterator &operator=(BackwardIterator &&other) = default;

  BackwardIterator(T *pos)
    : pos_(pos)
  {}
  BackwardIterator(T *pos, T *begin)
    : pos_(pos)
    , begin_(begin)
  {}

  BackwardIterator &operator++()
  {
    --pos_;
    return *this;
  }

  T &operator*()
  {
    assert(pos_ != nullptr);
    assert((begin_ != nullptr) && (pos_ > begin_));
    return *pos_;
  }

  bool operator==(BackwardIterator const &other) const
  {
    return other.pos_ == pos_;
  }
  bool operator!=(BackwardIterator const &other) const
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
struct iterator_traits<fetch::memory::ForwardIterator<T>>
{
  using difference_type   = std::ptrdiff_t;
  using value_type        = std::remove_cv_t<T>;
  using pointer           = T *;
  using reference         = T &;
  using iterator_category = std::forward_iterator_tag;
};

template <typename T>
struct iterator_traits<fetch::memory::BackwardIterator<T>>
{
  using difference_type   = std::ptrdiff_t;
  using value_type        = std::remove_cv_t<T>;
  using pointer           = T *;
  using reference         = T &;
  using iterator_category = std::forward_iterator_tag;
};

}  // namespace std
