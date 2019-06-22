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

#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

#include "storage/key_byte_array_store.hpp"

namespace fetch {
namespace storage {

/**
 * Stores objects of type T using ResourceID as a key, writing to disk, hence
 * the New/Load
 * functions.
 *
 * Note that you should be using ResourceAddress to hash to a ResourceID
 * otherwise you will
 * get key collisions. See the test object_store.cpp for usage
 *
 * Since the objects are stored to disk, you must have defined a serializer and
 * deserializer for
 * the type T you want to store. See object_store.cpp for an example
 *
 * S is the document store's underlying block size
 *
 */
template <typename T, std::size_t S = 2048>
class ObjectStore
{
public:
  using type            = T;
  using self_type       = ObjectStore<T, S>;
  using serializer_type = serializers::TypedByteArrayBuffer;

  class Iterator;

  ObjectStore()                       = default;
  ObjectStore(ObjectStore const &rhs) = delete;
  ObjectStore(ObjectStore &&rhs)      = delete;
  ObjectStore &operator=(ObjectStore const &rhs) = delete;
  ObjectStore &operator=(ObjectStore &&rhs) = delete;

  /**
   * Create a new file for the object store with the filename parameters for the
   * document,
   * and the index to it.
   *
   * If these arguments correspond to existing files, it will overwrite them
   */
  void New(std::string const &doc_file, std::string const &index_file,
           bool const & /*create*/ = true)
  {
    store_.New(doc_file, index_file);
  }

  /**
   * Load a file into the document store with the filename parameters for the
   * document,
   * and the index to it
   */
  void Load(std::string const &doc_file, std::string const &index_file, bool const &create = true)
  {
    store_.Load(doc_file, index_file, create);
  }

  /**
   * Assign object given a key (ResourceID)
   *
   * @param: rid The key
   * @param: object The object to assign
   *
   * @return: whether the operation was successful
   */
  bool Get(ResourceID const &rid, type &object)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return LocklessGet(rid, object);
  }

  void Erase(ResourceID const &rid)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    LocklessErase(rid);
  }

  /**
   * Check whether a key has been set
   *
   * @param: rid The key
   *
   * @return: whether the key has been set
   */
  bool Has(ResourceID const &rid)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return LocklessHas(rid);
  }

  /**
   * Put object into the store using the key
   *
   * @param: rid The key
   * @param: object The object
   *
   */
  void Set(ResourceID const &rid, type const &object)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    LocklessSet(rid, object);
  }

  /**
   * Obtain a lock then execute closure to reduce overhead from requiring
   * multiple locks to be
   * acquired
   *
   * @param: f The closure
   */
  template <typename F>
  void WithLock(F &&f)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    f();
  }

  /**
   * Do a get without locking the structure, do this when it is guaranteed you
   * have locked (using
   * WithLock) or don't need to lock (single threaded scenario)
   *
   * @param: rid The key
   * @param: object The object
   *
   * @return whether the get was successful
   */
  bool LocklessGet(ResourceID const &rid, type &object)
  {
    // assert(object != nullptr);
    Document doc = store_.Get(rid);
    if (doc.failed)
    {
      return false;
    }

    serializer_type ser(doc.document);

    ser >> object;

    return true;
  }

  void LocklessErase(ResourceID const &rid)
  {
    store_.Erase(rid);
  }

  /**
   * Do a has without locking the structure, do this when it is guaranteed you
   * have locked (using
   * WithLock) or don't need to lock (single threaded scenario)
   *
   * @param: rid The key
   * @param: object The object
   *
   * @return whether the has was successful
   */
  bool LocklessHas(ResourceID const &rid)
  {
    Document doc = store_.Get(rid);
    return !doc.failed;
  }

  /**
   * Do a set without locking the structure, do this when it is guaranteed you
   * have locked (using
   * WithLock) or don't need to lock (single threaded scenario)
   *
   * @param: rid The key
   * @param: object The object
   *
   */
  void LocklessSet(ResourceID const &rid, type const &object)
  {
    serializer_type ser;
    ser << object;

    store_.Set(rid, ser.data());  // temporarily disable disk writes
  }

  std::size_t size() const
  {
    return store_.size();
  }

  void Flush(bool lazy = true)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    store_.Flush(lazy);
  }

  /**
   * STL-like functionality achieved with an iterator class. This has to wrap an
   * iterator to the
   * KeyByteArrayStore since we need to deserialize at this level to return the
   * object
   */
  class Iterator
  {
  public:
    Iterator(typename KeyByteArrayStore<S>::Iterator it)
      : wrapped_iterator_{it}
    {}
    Iterator()                    = default;
    Iterator(Iterator const &rhs) = default;
    Iterator(Iterator &&rhs)      = default;
    Iterator &operator=(Iterator const &rhs) = default;
    Iterator &operator=(Iterator &&rhs) = default;

    void operator++()
    {
      ++wrapped_iterator_;
    }

    bool operator==(Iterator const &rhs) const
    {
      return wrapped_iterator_ == rhs.wrapped_iterator_;
    }

    bool operator!=(Iterator const &rhs) const
    {
      return !(wrapped_iterator_ == rhs.wrapped_iterator_);
    }

    ResourceID GetKey() const
    {
      return ResourceID{wrapped_iterator_.GetKey()};
    }

    /**
     * Dereference operator
     *
     * @return: a deserialized object
     */
    type operator*() const
    {
      Document doc = *wrapped_iterator_;

      type            ret;
      serializer_type ser(doc.document);
      ser >> ret;

      return ret;
    }

  protected:
    typename KeyByteArrayStore<S>::Iterator wrapped_iterator_;
  };

  self_type::Iterator Find(ResourceID const &rid)
  {
    auto it = store_.Find(rid);

    return Iterator(it);
  }

  /**
   * Get an iterator to the first element of a subtree (the first element of the
   * range that
   * matches the first bits of rid)
   *
   * @param: rid The key
   * @param: bits The number of bits of rid we want to match against
   *
   * @return: an iterator to the first element of that tree
   */
  self_type::Iterator GetSubtree(ResourceID const &rid, uint64_t bits)
  {
    auto it = store_.GetSubtree(rid, bits);

    return Iterator(it);
  }

  self_type::Iterator begin()
  {
    return Iterator(store_.begin());
  }

  self_type::Iterator end()
  {
    return Iterator(store_.end());
  }

private:
  mutex::Mutex         mutex_{__LINE__, __FILE__};
  KeyByteArrayStore<S> store_;
};

}  // namespace storage
}  // namespace fetch
