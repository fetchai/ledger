#pragma once
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

#include "storage/key_byte_array_store.hpp"
// #include "core/serialize/"
namespace fetch {
namespace storage {

template <typename T, std::size_t S = 2048>
class ObjectStore
{
public:
  using type            = T;
  using self_type       = ObjectStore<T, S>;
  using serializer_type = serializers::TypedByteArrayBuffer;
  class iterator;

  template <typename... Args>
  void New(Args &&... args)
  {
    store_.New(std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Load(Args &&... args)
  {
    store_.Load(std::forward<Args>(args)...);
  }

  void Clear() { store_.Clear(); }

  void Flush() { store_.Flush(); }

  bool is_open() const { return store_.is_open(); }

  bool empty() const { return store_.empty(); }

  void Close() { store_.Close(); }

  void WithLock(std::function<void()> const &f)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    f();
  }

  bool LocklessGet(ResourceID const &rid, type &object)
  {
    Document doc = store_.Get(rid);
    if (doc.failed) return false;

    serializer_type ser(doc.document);
    ser >> object;
    return true;
  }

  bool LocklessHas(ResourceID const &rid)
  {
    Document doc = store_.Get(rid);
    return !doc.failed;
  }

  void LocklessSet(ResourceID const &rid, type const &object)
  {
    serializer_type ser;
    ser << object;

    store_.Set(rid, ser.data());
  }

  bool Get(ResourceID const &rid, type &object)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return LocklessGet(rid, object);
  }

  bool Has(ResourceID const &rid)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return LocklessHas(rid);
  }

  void Set(ResourceID const &rid, type const &object)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return LocklessSet(rid, object);
  }

  // STL-like functionality
  // template <typename... Args>
  self_type::iterator Find(ResourceID const &rid)
  {
    auto it = store_.Find(rid);

    return iterator(it);
  }

  template <typename... Args>
  self_type::iterator GetSubtree(Args &&... args)
  {
    auto it = store_.GetSubtree(std::forward<Args>(args)...);

    return iterator(it);
  }

  self_type::iterator begin() { return iterator(store_.begin()); }
  self_type::iterator end() { return iterator(store_.end()); }

  class iterator
  {
  public:
    iterator(typename KeyByteArrayStore<S>::iterator it) : wrapped_iterator_{it} {}
    iterator()                    = default;
    iterator(iterator const &rhs) = default;
    iterator(iterator &&rhs)      = default;
    iterator &operator=(iterator const &rhs) = default;
    iterator &operator=(iterator &&rhs) = default;

    void operator++() { ++wrapped_iterator_; }

    bool operator==(iterator const &rhs) { return wrapped_iterator_ == rhs.wrapped_iterator_; }
    bool operator!=(iterator const &rhs) { return !(wrapped_iterator_ == rhs.wrapped_iterator_); }

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

private:
  mutex::Mutex mutex_{ __LINE__, __FILE__ };
  KeyByteArrayStore<S> store_;
};

}  // namespace storage
}  // namespace fetch
