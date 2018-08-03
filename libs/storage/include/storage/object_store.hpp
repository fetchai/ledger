#pragma once
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

#include "storage/key_byte_array_store.hpp"

namespace fetch {
namespace storage {

/**
* Stores objects of type T using ResourceID as a key, writing to disk, hence the New/Load
* functions.
*
* Note that you should be using ResourceAddress to hash to a ResourceID otherwise you will
* get key collisions. See the test object_store.cpp for usage
*
* Since the objects are stored to disk, you must have defined a serializer and deserializer for 
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
  class iterator;

  /**
  * Create a new file(s) with these args
  */
  template <typename... Args>
  void New(Args &&... args)
  {
    store_.New(std::forward<Args>(args)...);
  }

  /**
  * Load a file(s) with these args
  */
  template <typename... Args>
  void Load(Args &&... args)
  {
    store_.Load(std::forward<Args>(args)...);
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
    return LocklessSet(rid, object);
  }

  /**
  * Obtain a lock then execute closure to reduce overhead from requiring multiple locks to be 
  * acquired
  *
  * @param: f The closure
  */
  void WithLock(std::function<void()> const &f)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    f();
  }

  /**
  * Do a get without locking the structure, do this when it is guaranteed you have locked (using
  * WithLock) or don't need to lock (single threaded scenario)
  *
  * @param: rid The key
  * @param: object The object
  *
  * @return whether the get was successful
  */
  bool LocklessGet(ResourceID const &rid, type &object)
  {
    Document doc = store_.Get(rid);
    if (doc.failed) return false;

    serializer_type ser(doc.document);
    ser >> object;
    return true;
  }

  /**
  * Do a has without locking the structure, do this when it is guaranteed you have locked (using
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
  * Do a set without locking the structure, do this when it is guaranteed you have locked (using
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

    store_.Set(rid, ser.data());
  }

  /**
  * STL-like functionality achieved with an iterator class. This has to wrap an iterator to the
  * key value store since we need to deserialize at this level to return the object
  */
  class iterator
  {
  public:
    iterator(typename KeyByteArrayStore<S>::iterator it) : wrapped_iterator_{it} {}
    iterator()                               = default;
    iterator(iterator const &rhs)            = default;
    iterator(iterator &&rhs)                 = default;
    iterator &operator=(iterator const &rhs) = default;
    iterator &operator=(iterator &&rhs)      = default;

    /**
    * Pre increment operator
    */
    void operator++() { ++wrapped_iterator_; }

    /**
    * equal to operator
    */
    bool operator==(iterator const &rhs) { return wrapped_iterator_ == rhs.wrapped_iterator_; }

    /**
    * not equal to operator
    */
    bool operator!=(iterator const &rhs) { return !(wrapped_iterator_ == rhs.wrapped_iterator_); }

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
    typename KeyByteArrayStore<S>::iterator wrapped_iterator_;
  };

  /**
  * STL-like find, given a key
  *
  * @param: rid The key
  *
  * @return: An iterator to the element
  */
  self_type::iterator Find(ResourceID const &rid)
  {
    auto it = store_.Find(rid);

    return iterator(it);
  }

  /**
  * Get an iterator to the first element of a subtree (the first element of the range that
  * matches the first bits of rid)
  *
  * @param: rid The key
  * @param: bits The number of bits of rid we want to match against
  *
  * @return: an iterator to the first element of that tree
  */
  self_type::iterator GetSubtree(ResourceID const &rid, uint64_t bits)
  {
    auto it = store_.GetSubtree(rid, bits);

    return iterator(it);
  }

  /**
  * Iterator to beginning of object store
  *
  * @return: iterator to begin
  */
  self_type::iterator begin() { return iterator(store_.begin()); }

  /**
  * Iterator to end of object store
  *
  * @return: iterator to end
  */
  self_type::iterator end() { return iterator(store_.end()); }


private:
  mutex::Mutex         mutex_;
  KeyByteArrayStore<S> store_;
};

}  // namespace storage
}  // namespace fetch
