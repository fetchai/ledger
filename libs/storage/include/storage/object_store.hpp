#ifndef STORAGE_OBJECT_STORE_HPP
#define STORAGE_OBJECT_STORE_HPP
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

#include "storage/key_byte_array_store.hpp"
// #include "core/serialize/"
namespace fetch {
namespace storage {

template<typename T, std::size_t S = 2048>
class ObjectStore
{
public:
  typedef T type;
  typedef serializers::TypedByte_ArrayBuffer serializer_type;
  
  template<typename... Args>
  void New(Args &&...args) 
  {
    store_.New(std::forward<Args>(args)...);      
  }
    
  template<typename... Args>
  void Load(Args &&...args) 
  {
    store_.Load(std::forward<Args>(args)...);      
  }

  void Clear() 
  {
    store_.Clear();
  }
  
  void Flush()
  {
    store_.Flush();
  }
  
  bool is_open() const 
  {
    return store_.is_open();
  }
  
  bool empty() const 
  {
    return store_.empty();
  }
  
  void Close() 
  {
    store_.Close();
  }
  
  bool Get(ResourceID const &rid, type &object) 
  {
    Document doc = store_.Get(rid);
    if(doc.failed) return false;

    serializer_type ser(doc.document);
    ser >> object;
    return true;
    
  }

  void Set(ResourceID const &rid, type const &object) 
  {    
    serializer_type ser;
    ser << object;

    store_.Set(rid, ser.data());
  }
  
private:

  KeyByteArrayStore< S > store_;
  
};

}
}

#endif
