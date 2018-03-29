#ifndef SERIALIZER_REFERENCED_BYTE_ARRAY_HPP
#define SERIALIZER_REFERENCED_BYTE_ARRAY_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "byte_array/const_byte_array.hpp"
#include "assert.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {

template <typename T>
void Serialize(T &serializer, byte_array::BasicByteArray const &s) {
  serializer.Allocate(sizeof(uint64_t) + s.size());
  uint64_t size = s.size();

  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(s.pointer()),
                        s.size());
}

template <typename T>
void Deserialize(T &serializer, byte_array::BasicByteArray &s) {
  uint64_t size = 0;
  if(!(sizeof(uint64_t) <= serializer.bytes_left())){
    std::cout << serializer.bytes_left() << " " << serializer.data().size() << std::endl;
  }
  detailed_assert( sizeof(uint64_t) <= serializer.bytes_left());  
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));
  detailed_assert( size <= serializer.bytes_left());  

  serializer.ReadByteArray(s, size);
  //  s.Resize(size);
  //  serializer.ReadBytes(reinterpret_cast<uint8_t *>(s.pointer()), s.size());
}


};
};

#endif
