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

#include <memory>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace variant {
namespace detail {

/**
 * Basic cached object store. Useful for reusing / pre-allocating objects will be used regularly
 *
 * @note This is not thread safe, all implementations expect to used in the content of one thread
 *
 * @tparam T The type of the cached object.
 */
template <typename T>
class ElementPool
{
public:
  // Construction / Destruction
  ElementPool() = default;
  explicit ElementPool(std::size_t size);
  ElementPool(ElementPool const &) = delete;
  ElementPool(ElementPool &&)      = default;
  ~ElementPool()                   = default;

  T *  Allocate();
  void Release(T *element);
  bool empty() const;

  // Operators
  ElementPool &operator=(ElementPool const &) = delete;
  ElementPool &operator=(ElementPool &&) = default;

private:
  static constexpr std::size_t DEFAULT_ALLOCATE_BATCH = 10;

  void Reserve(std::size_t size);

  using ElementPtr     = std::shared_ptr<T>;
  using UnderlyingPool = std::vector<ElementPtr>;
  using ElementList    = std::vector<T *>;
  using ElementSet     = std::unordered_set<T *>;

  UnderlyingPool pool_;
  ElementList    free_;
  ElementSet     in_use_;
};

/**
 * Construct the element pool
 *
 * @tparam T The type of the cached object
 * @param size The number of objects to pre-allocate
 */
template <typename T>
ElementPool<T>::ElementPool(std::size_t size)
{
  Reserve(size);
}

/**
 * Allocate a object from the pool
 *
 * @tparam T The type of the cached object
 * @return The pointer to the new object from the pool
 */
template <typename T>
T *ElementPool<T>::Allocate()
{
  // if the free queue is empty then allocate some new instances into the pool
  if (free_.empty())
  {
    Reserve(DEFAULT_ALLOCATE_BATCH);
  }

  // sanity check: the previous operation should ensure the pool is populated
  if (free_.empty())
  {
    throw std::runtime_error("Unable to allocate element");
  }

  // lookup the next element and update the internal data structures
  T *element = free_.back();
  free_.pop_back();
  in_use_.emplace(element);

  return element;
}

/**
 * Release an previously acquired object and return it to the pool
 *
 * @tparam T The type of the cached object
 * @param element The pointer to the element being returned
 */
template <typename T>
void ElementPool<T>::Release(T *element)
{
  if (in_use_.find(element) == in_use_.end())
  {
    throw std::runtime_error("Element is not part of this pool");
  }
  else
  {
    in_use_.erase(element);
    free_.push_back(element);
  }
}

/**
 * Check to see if the pool is empty
 *
 * @tparam T The type of the cached object
 * @return true if the pool is empty, otherwise false
 */
template <typename T>
inline bool ElementPool<T>::empty() const
{
  return pool_.empty();
}

/**
 * Internal: Reserve a number of objects and add them to the free pool
 *
 * @tparam T The type of the cached object
 * @param size The number of element to pre-allocate
 */
template <typename T>
void ElementPool<T>::Reserve(std::size_t size)
{
  for (std::size_t i = 0; i < size; ++i)
  {
    pool_.emplace_back(std::make_unique<T>());
    free_.emplace_back(pool_.back().get());
  }
}

}  // namespace detail
}  // namespace variant
}  // namespace fetch
