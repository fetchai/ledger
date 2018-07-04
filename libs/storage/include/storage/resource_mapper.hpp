#ifndef STORAGE_RESOURCE_MAPPERL_HPP
#define STORAGE_RESOURCE_MAPPERL_HPP

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include"core/byte_array/encoders.hpp"
#include "core/assert.hpp"

namespace fetch {
namespace storage {

class ResourceID 
{
public:
  typedef uint32_t resource_group_type;
  ResourceID() { }

  ResourceID(byte_array::ByteArray &id)
  {
    id_ = id;
  }

  ResourceID(byte_array::ConstByteArray const &id) 
  {
    id_ = id;
  }
  
  byte_array::ConstByteArray id() const 
  {
    return id_;
  }

  resource_group_type const & resource_group() const {
    detailed_assert(id_.size() > sizeof(uint64_t));
    return *reinterpret_cast< resource_group_type const* >( id_.pointer() );
  }  

  resource_group_type lane(resource_group_type const &log2_num_lanes) const {
    detailed_assert(log2_num_lanes < (sizeof(resource_group_type) * 8));

    // define the group
    resource_group_type const group_mask = (1u << log2_num_lanes) - 1u;

    return resource_group() & group_mask;
  }
  
private:
  byte_array::ConstByteArray id_;
  template <typename T>
  friend inline void Serialize(T &, ResourceID const &);
  template <typename T>
  friend inline void Deserialize(T &, ResourceID &);  
};

template <typename T>
void Serialize(T &serializer, ResourceID const &b) {
  serializer << b.id_;
}

template <typename T>
void Deserialize(T &serializer, ResourceID &b) {
  serializer >> b.id_;
}

class ResourceAddress : public ResourceID
{
public:
  ResourceAddress(byte_array::ByteArray const &address)
    : ResourceID( crypto::Hash< crypto::SHA256 >(address) )  
  {
    address_ = address;
  }

  byte_array::ConstByteArray address() const 
  {
    return address_;
  }
  
private:  
  byte_array::ByteArray address_;
};




}
}

#endif
