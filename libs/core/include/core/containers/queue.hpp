#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/mutex.hpp"
#include "core/sync/tickets.hpp"
#include "meta/is_log2.hpp"

#include <array>
#include <deque>

namespace fetch {
namespace core {

/**
 * A Single threaded index
 *
 * @tparam SIZE
 */
template <std::size_t SIZE>
class SingleThreadedIndex
{
public:
  // Construction / Destruction
  explicit SingleThreadedIndex(std::size_t initial)
    : index_(initial)
  {}
  SingleThreadedIndex(SingleThreadedIndex const &) = delete;
  SingleThreadedIndex(SingleThreadedIndex &&)      = delete;
  ~SingleThreadedIndex()                           = default;

  /**
   * Post increment operator
   *
   * @return The new index value
   */
  std::size_t operator++(int)
  {
    std::size_t index = index_++;
    index_ &= MASK;

    return index;
  }

  // Operators
  SingleThreadedIndex &operator=(SingleThreadedIndex const &) = delete;
  SingleThreadedIndex &operator=(SingleThreadedIndex &&) = delete;

private:
  static constexpr std::size_t MASK = SIZE - 1;

  std::size_t index_{0};

  // static assertions
  static_assert(meta::IsLog2<SIZE>::value, "Queue size must be a valid power of 2");
};

/**
 * A Multi threaded index
 *
 * @tparam SIZE
 */
template <std::size_t SIZE>
class MultiThreadedIndex : protected SingleThreadedIndex<SIZE>
{
public:
  // Construction / Destruction
  explicit MultiThreadedIndex(std::size_t initial)
    : SingleThreadedIndex<SIZE>(initial)
  {}
  ~MultiThreadedIndex() = default;

  /**
   * Post increment operator
   *
   * @return The new index value
   */
  std::size_t operator++(int)
  {
    std::lock_guard<std::mutex>       lock(lock_);
    return SingleThreadedIndex<SIZE>::operator++(1);
  }

private:
  std::mutex lock_;
};

/**
 * Single Producer, Single Consumer fixed length queue
 *
 * @tparam T The type of element to be store in the queue
 * @tparam SIZE The max size of the queue
 * @tparam Producer The thread safety model for the producer size of the queue
 * @tparam Consumer The thread safety model for the consumer size of the queue
 */
template <typename T, std::size_t SIZE, typename ProducerIndex = MultiThreadedIndex<SIZE>,
          typename ConsumerIndex = MultiThreadedIndex<SIZE>>
class Queue
{
public:
  static constexpr std::size_t QUEUE_LENGTH = SIZE;

  using Element = T;

  // Construction / Destruction
  Queue()              = default;
  Queue(Queue const &) = delete;
  Queue(Queue &&)      = delete;
  ~Queue()             = default;

  /// @name Queue Interaction
  /// @{
  T Pop();
  template <typename R, typename P>
  bool Pop(T &value, std::chrono::duration<R, P> const &duration);
  void Push(T const &element);
  void Push(T &&element);
  /// @}

  // Operators
  Queue &operator=(Queue const &) = delete;
  Queue &operator=(Queue &&) = delete;

protected:
  using Array = std::array<T, SIZE>;

  Array         queue_;              ///< The main element container
  ProducerIndex write_index_{0};     ///< The write index
  ConsumerIndex read_index_{0};      ///< The read index
  Tickets       read_count_{0};      ///< The read semaphore/tickets object
  Tickets       write_count_{SIZE};  ///< The write semaphore/tickets object

  // static asserts
  static_assert(meta::IsLog2<SIZE>::value, "Queue size must be a valid power of 2");
  static_assert(std::is_default_constructible<T>::value, "T must be default constructable");
  static_assert(std::is_copy_assignable<T>::value, "T must have copy assignment");
};

/**
 * Pop an element from the queue
 *
 * If no element is available then the function will block until an element
 * is available.
 *
 * @tparam T The type of element to be store in the queue
 * @tparam N The max size of the queue
 * @tparam P The producer index type
 * @tparam C The consumer index type
 * @return The element retrieved from the queue
 */
template <typename T, std::size_t N, typename P, typename C>
T Queue<T, N, P, C>::Pop()
{
  read_count_.Wait();

  T value = queue_[read_index_++];

  write_count_.Post();

  return value;
}

/**
 * Pop an element from the queue with a specified maximum wait duration
 *
 * @tparam T The type of element to be store in the queue
 * @tparam N The max size of the queue
 * @tparam P The producer index type
 * @tparam C The consumer index type
 * @tparam Rep The tick representation for the duration
 * @tparam Per The tick period for the duration
 * @param value The reference to the value to be populated
 * @param duration The maximum amount of time to wait for an element
 * @return true if an element was extracted, otherwise false
 */
template <typename T, std::size_t N, typename P, typename C>
template <typename Rep, typename Per>
bool Queue<T, N, P, C>::Pop(T &value, std::chrono::duration<Rep, Per> const &duration)
{
  if (!read_count_.Wait(duration))
  {
    return false;
  }

  value = queue_[read_index_++];

  write_count_.Post();

  return true;
}

/**
 * Push an element onto the queue
 *
 * If the queue is full this function will block until an element can be added
 *
 * @tparam T The type of element to be store in the queue
 * @tparam N The max size of the queue
 * @tparam P The producer index type
 * @tparam C The consumer index type
 * @param element The reference to the element
 */
template <typename T, std::size_t N, typename P, typename C>
void Queue<T, N, P, C>::Push(T const &element)
{
  write_count_.Wait();

  queue_[write_index_++] = element;

  read_count_.Post();
}

/**
 * Push an element onto the queue
 *
 * If the queue is full this function will block until an element can be added
 *
 * @tparam T The type of element to be store in the queue
 * @tparam N The max size of the queue
 * @tparam P The producer index type
 * @tparam C The consumer index type
 * @param element The reference to the element
 */
template <typename T, std::size_t N, typename P, typename C>
void Queue<T, N, P, C>::Push(T &&element)
{
  write_count_.Wait();

  queue_[write_index_++] = std::move(element);

  read_count_.Post();
}

// Helpful Typedefs
template <typename T, std::size_t N>
using SPSCQueue = Queue<T, N, SingleThreadedIndex<N>, SingleThreadedIndex<N>>;

template <typename T, std::size_t N>
using SPMCQueue = Queue<T, N, SingleThreadedIndex<N>, MultiThreadedIndex<N>>;

template <typename T, std::size_t N>
using MPSCQueue = Queue<T, N, MultiThreadedIndex<N>, SingleThreadedIndex<N>>;

template <typename T, std::size_t N>
using MPMCQueue = Queue<T, N, MultiThreadedIndex<N>, MultiThreadedIndex<N>>;

}  // namespace core
}  // namespace fetch
