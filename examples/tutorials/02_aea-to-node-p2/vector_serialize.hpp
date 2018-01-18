#ifndef VECTOR_SERIALIZE_HPP
#define VECTOR_SERIALIZE_HPP
#include<vector>
namespace fetch {
namespace serializers {
    
template< typename T>
void Serialize(T &serializer, std::vector< double > const &vec) {
  uint64_t bytes = vec.size() * sizeof(double);

  // Allocating bytes
  serializer.Allocate( sizeof(uint64_t) + bytes);
  uint64_t size = vec.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));

  // Writing the elements
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(vec.data()),
                        bytes);
}

template< typename T>
void Deserialize(T &serializer, std::vector< double > &vec) {
  uint64_t size;
  // Writing the size to the byte array
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size),
                        sizeof(uint64_t));

  // Reading the data
  uint64_t bytes = size * sizeof(double);
  vec.resize( size );
  serializer.WriteBytes(reinterpret_cast<uint8_t *>(vec.data()),
                        bytes);  

}

};
};
#endif
