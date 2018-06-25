#ifndef VECTOR_SERIALIZE_HPP
#define VECTOR_SERIALIZE_HPP
#include"core/serializers/stl_types.hpp"
#include<vector>
namespace fetch {
namespace serializers {
    
template< typename T>
void Serialize(T &serializer, std::vector< std::string > const &vec) {
  // Allocating memory for the size
  serializer.Allocate( sizeof(uint64_t) );
  uint64_t size = vec.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));

  for( auto const &a : vec ) serializer << a;
}

template< typename T>
void Deserialize(T &serializer, std::vector< std::string > &vec) {
  uint64_t size;
  // Writing the size to the byte array
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size),
                        sizeof(uint64_t));

  // Reading the data
  vec.resize( size );

  for( auto &a : vec ) 
    serializer >> a;

}

}
}
#endif
