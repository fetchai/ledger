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

#include <array>
#include <cstddef>

namespace fetch {
namespace core {

// Forward Declarations
template <typename Value, std::size_t LENGTH>
class HistoryBuffer;

/**
 * Iterator to be used with the history buffer
 *
 * @tparam Value The element value type
 * @tparam LENGTH The maximum size of the history buffer
 */
template <typename Value, std::size_t LENGTH>
class HistoryBufferIterator
{
public:
  // Construction / Destruction
  explicit HistoryBufferIterator(HistoryBuffer<Value, LENGTH> const &buffer,
                                 std::size_t                         offset = 0);
  HistoryBufferIterator(HistoryBufferIterator const &) = default;
  ~HistoryBufferIterator()                             = default;

  // Read Only Access Operators
  HistoryBufferIterator &operator++();
  Value const &          operator*() const;
  Value const *          operator->() const;
  bool                   operator==(HistoryBufferIterator const &other) const;
  bool                   operator!=(HistoryBufferIterator const &other) const;

private:
  HistoryBuffer<Value, LENGTH> const *read_only_{nullptr};
  std::size_t                         offset_{0};
};

/**
 * Helper container used to keep track of a fixed number of items
 *
 * @tparam Value The element value type
 * @tparam LENGTH The maximum size of the history buffer
 */
template <typename Value, std::size_t LENGTH>
class HistoryBuffer
{
public:
  using iterator       = HistoryBufferIterator<Value, LENGTH>;
  using const_iterator = iterator;

  // Construction / Destruction
  HistoryBuffer()                          = default;
  HistoryBuffer(HistoryBuffer const &)     = delete;
  HistoryBuffer(HistoryBuffer &&) noexcept = delete;
  ~HistoryBuffer()                         = default;

  // Read Only Iterator Access
  const_iterator cbegin() const;
  const_iterator begin() const;
  const_iterator cend() const;
  const_iterator end() const;

  bool        empty() const;
  std::size_t size() const;

  Value const &operator[](std::size_t offset) const;
  Value const &at(std::size_t offset) const;

  void emplace_back(Value const &value);
  void emplace_back(Value &&value);

private:
  using UnderlyingArray = std::array<Value, LENGTH>;

  static constexpr std::size_t LAST_INDEX = LENGTH - 1;

  UnderlyingArray buffer_;              ///< Storage of the objects
  std::size_t     size_{0};             ///< The total number of stored objects
  std::size_t     offset_{LAST_INDEX};  ///< The current start index

  friend HistoryBufferIterator<Value, LENGTH>;
};

/**
 * Begin Iterator Accessor
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return The iterator to be begining of the buffer
 */
template <typename V, std::size_t L>
typename HistoryBuffer<V, L>::const_iterator HistoryBuffer<V, L>::cbegin() const
{
  return HistoryBufferIterator<V, L>{*this};
}

/**
 * Begin Iterator Accessor
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return The iterator to be begining of the buffer
 */
template <typename V, std::size_t L>
typename HistoryBuffer<V, L>::const_iterator HistoryBuffer<V, L>::begin() const
{
  return HistoryBufferIterator<V, L>{*this};
}

/**
 * End Iterator Accessor
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return The iterator to be end of the buffer
 */
template <typename V, std::size_t L>
typename HistoryBuffer<V, L>::const_iterator HistoryBuffer<V, L>::cend() const
{
  return HistoryBufferIterator<V, L>{*this, size()};
}

/**
 * End Iterator Accessor
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return The iterator to be end of the buffer
 */
template <typename V, std::size_t L>
typename HistoryBuffer<V, L>::const_iterator HistoryBuffer<V, L>::end() const
{
  return HistoryBufferIterator<V, L>{*this, size()};
}

/**
 * Check to see if the buffer if empty
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return true if empty, otherwise false
 */
template <typename V, std::size_t L>
bool HistoryBuffer<V, L>::empty() const
{
  return size_ == 0;
}

/**
 * Get the size of the history buffer
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return The number of elements present
 */
template <typename V, std::size_t L>
std::size_t HistoryBuffer<V, L>::size() const
{
  return size_;
}

/**
 * Access a specific element of the buffer (without bounds checking)
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @param offset The offset being accessed
 * @return The reference to the accessed element
 */
template <typename V, std::size_t L>
V const &HistoryBuffer<V, L>::operator[](std::size_t offset) const
{
  return buffer_[(offset_ + offset + 1) % L];
}

/**
 * Access a specific element of the buffer (with bounds checking)
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @param offset The offset being accessed
 * @return The reference to the accessed element
 */
template <typename V, std::size_t L>
V const &HistoryBuffer<V, L>::at(std::size_t offset) const
{
  if (offset >= size_)
  {
    throw std::runtime_error("History buffer access out of range");
  }

  return (*this)[offset];
}

/**
 * Add (copy) a new element into the history buffer
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @param value The value to be copied
 */
template <typename V, std::size_t L>
void HistoryBuffer<V, L>::emplace_back(V const &value)
{
  buffer_[offset_] = value;
  offset_          = (offset_ + LAST_INDEX) % L;
  size_            = std::min(size_ + 1, L);
}

/**
 * Add (move) a new element into the history buffer
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @param value The value to be moved
 */
template <typename V, std::size_t L>
void HistoryBuffer<V, L>::emplace_back(V &&value)
{
  buffer_[offset_] = std::move(value);
  offset_          = (offset_ + LAST_INDEX) % L;
  size_            = std::min(size_ + 1, L);
}

/**
 * Create a history buffer iterator from a given buffer with a specified offset
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @param buffer THe reference to the buffer being iterated over
 * @param offset The offset to be used
 */
template <typename V, std::size_t L>
HistoryBufferIterator<V, L>::HistoryBufferIterator(HistoryBuffer<V, L> const &buffer,
                                                   std::size_t                offset)
  : read_only_{&buffer}
  , offset_{std::min(offset, read_only_->size())}
{}

/**
 * Increment iterator position
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return The reference to the iterator
 */
template <typename V, std::size_t L>
HistoryBufferIterator<V, L> &HistoryBufferIterator<V, L>::operator++()
{
  offset_ = std::max(offset_ + 1, read_only_->size());
  return *this;
}

/**
 * Dereference the iterator and access the current item
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return The reference to the current item
 */
template <typename V, std::size_t L>
V const &HistoryBufferIterator<V, L>::operator*() const
{
  return *operator->();
}

/**
 * Dereference the iterator and access the current item
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @return The pointer to the current item
 */
template <typename V, std::size_t L>
V const *HistoryBufferIterator<V, L>::operator->() const
{
  return &(read_only_->at(offset_));
}

/**
 * Compare two iterators for equality
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @param other The other iterator to compare against
 * @return true if equal otherwise false
 */
template <typename V, std::size_t L>
bool HistoryBufferIterator<V, L>::operator==(HistoryBufferIterator const &other) const
{
  return (other.read_only_ == read_only_) && (offset_ == other.offset_);
}

/**
 * Compare two iterators for inequality
 *
 * @tparam V The value type
 * @tparam L The maximum length of the history buffer
 * @param other The other iterator to compare against
 * @return true if different, otherwise false
 */
template <typename V, std::size_t L>
bool HistoryBufferIterator<V, L>::operator!=(HistoryBufferIterator const &other) const
{
  return !(*this == other);
}

}  // namespace core
}  // namespace fetch
