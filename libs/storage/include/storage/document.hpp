#ifndef STORAGE_DOCUMENT_HPP
#define STORAGE_DOCUMENT_HPP

#include "core/byte_array/const_byte_array.hpp"

namespace fetch {
namespace storage {

struct Document
{
  explicit operator byte_array::ConstByteArray()  {
    return document;
  }
  
  byte_array::ByteArray document;
  bool was_created = false;
  bool failed = false;
};


template <typename T>
void Serialize(T &serializer, Document const &b) {
  serializer << b.document << b.was_created << b.failed;
}

template <typename T>
void Deserialize(T &serializer, Document &b) {
  serializer >> b.document >> b.was_created >> b.failed;
}

 
}
}


#endif
