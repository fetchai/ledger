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
  static constexpr std::size_t max() noexcept
  {
    return MASK;
  }

  // Construction / Destruction
  explicit constexpr SingleThreadedIndex(std::size_t initial) noexcept
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
  constexpr std::size_t operator++(int) noexcept
  {
    std::size_t index = index_++;
    index_ &= MASK;

    return index;
  }

  // Operators
  constexpr SingleThreadedIndex &operator=(SingleThreadedIndex const &) = delete;
  constexpr SingleThreadedIndex &operator=(SingleThreadedIndex &&) = delete;

  constexpr std::size_t value() const noexcept
  {
    return index_;
  }
  template <class That>
  constexpr std::size_t operator-(That &&that) const noexcept(noexcept(that.value()))
  {
    static_assert(max() == that.max(), "RHS operand should take values from the same ring");

    std::size_t thatVal{that.value()};
    return index_ > thatVal ? index_ - thatVal : index_ + SIZE - thatVal;
  }

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
  static constexpr std::size_t max()
  {
    return SingleThreadedIndex<SIZE>::max();
  }

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

  std::size_t value()
  {
    std::lock_guard<std::mutex> lock(lock_);
    return SingleThreadedIndex<SIZE>::value();
  }
  template <class That>
  std::size_t operator-(That &&that)
  {
    static_assert(max() == that.max(), "RHS operand should take values from the same ring");

    std::lock_guard<std::mutex>       lock(lock_);
    return SingleThreadedIndex<SIZE>::operator-(that);
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
  bool        Pop(T &value, std::chrono::duration<R, P> const &duration);
  void        Push(T const &element);
  void        Push(T &&element);
  bool        empty();
  std::size_t size();
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
  static_assert(ProducerIndex::max() == ConsumerIndex::max(),
                "Indices should rotate in the same range");
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

template <typename T, std::size_t N, typename P, typename C>
bool Queue<T, N, P, C>::empty()
{
  return size() == 0;
}

template <typename T, std::size_t N, typename P, typename C>
std::size_t Queue<T, N, P, C>::size()
{
  return write_index_ - read_index_;
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

template <typename T, std::size_t SIZE>
class SimpleQueue
{
public:
  static constexpr std::size_t QUEUE_LENGTH = SIZE;
  using Element                             = T;

  // Construction / Destruction
  SimpleQueue()                    = default;
  SimpleQueue(SimpleQueue const &) = delete;
  SimpleQueue(SimpleQueue &&)      = delete;
  ~SimpleQueue()                   = default;

  /// @name Queue Interaction
  /// @{
  T Pop()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    T                            value;
    for (;;)
    {
      if (queue_.empty())
      {
        condition_.wait(lock);
      }
      else
      {
        // extract the value from the queue
        value = queue_.front();
        queue_.pop_front();
        // trigger all pending threads
        condition_.notify_all();
        break;
      }
    }
    return value;
  }

  template <typename R, typename P>
  bool Pop(T &value, std::chrono::duration<R, P> const &duration)
  {
    using Clock                           = std::chrono::high_resolution_clock;
    using Timestamp                       = Clock::time_point;
    Timestamp const              deadline = Clock::now() + duration;
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;)
    {
      if (queue_.empty())
      {
        auto const status = condition_.wait_until(lock, deadline);
        if (status == std::cv_status::timeout)
        {
          return false;
        }
      }
      else
      {
        // extract the value from the queue
        value = queue_.front();
        queue_.pop_front();
        // triggerr all pending threads
        condition_.notify_all();
        break;
      }
    }
    return true;
  }

  void Push(T const &element)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;)
    {
      if (queue_.size() >= SIZE)
      {
        condition_.wait(lock);
      }
      else
      {
        queue_.emplace_back(element);
        condition_.notify_all();
        break;
      }
    }
  }

  void Push(T &&element)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;)
    {
      if (queue_.size() >= SIZE)
      {
        condition_.wait(lock);
      }
      else
      {
        queue_.emplace_back(std::move(element));
        condition_.notify_all();
        break;
      }
    }
  }
  /// @}

  // Operators
  SimpleQueue &operator=(SimpleQueue const &) = delete;
  SimpleQueue &operator=(SimpleQueue &&) = delete;

protected:
  using Array = std::deque<T>;
  Array                   queue_;  ///< The main element container
  std::mutex              mutex_;
  std::condition_variable condition_;
  // static asserts
  static_assert(std::is_default_constructible<T>::value, "T must be default constructable");
  static_assert(std::is_copy_assignable<T>::value, "T must have copy assignment");
};

}  // namespace core
}  // namespace fetch
