#ifndef STORAGE_OBJECT_STORE_HPP
#define STORAGE_OBJECT_STORE_HPP
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
  typedef T                                  type;
  typedef serializers::TypedByte_ArrayBuffer serializer_type;

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

private:
  mutex::Mutex mutex_;

  KeyByteArrayStore<S> store_;
};

}  // namespace storage
}  // namespace fetch

#endif
