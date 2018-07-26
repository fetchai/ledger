#pragma once

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "crypto/fnv.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/assert.hpp"

#include <limits>

namespace fetch {
namespace storage {

class ResourceID 
{
public:
  typedef uint32_t resource_group_type;
  ResourceID() = default;

  ResourceID(byte_array::ConstByteArray const &id) 
  {
    set_id(id);
  }
  
  byte_array::ConstByteArray id() const 
  {
    return id_;
  }

  resource_group_type const & resource_group() const {
    return resource_group_;
  }  

  resource_group_type lane(resource_group_type const &log2_num_lanes) const {
    detailed_assert(log2_num_lanes < (sizeof(resource_group_type) * 8));

    // define the group
    resource_group_type const group_mask = (1u << log2_num_lanes) - 1u;

    return resource_group() & group_mask;
  }
  
private:

  void set_id(byte_array::ConstByteArray const &id) {
    crypto::CallableFNV hash;
    id_ = id;
    resource_group_ = static_cast<uint32_t>(hash(id));
  }

  byte_array::ConstByteArray id_;
  uint32_t resource_group_ = std::numeric_limits<uint32_t>::max();
  template <typename T>
  friend inline void Serialize(T &, ResourceID const &);
  template <typename T>
  friend inline void Deserialize(T &, ResourceID &);  
};

template <typename T>
void Serialize(T &serializer, ResourceID const &b) {
  serializer << b.id_ << b.resource_group_;
}

template <typename T>
void Deserialize(T &serializer, ResourceID &b) {
  serializer >> b.id_ >> b.resource_group_;
}

class ResourceAddress : public ResourceID
{
public:
  ResourceAddress(byte_array::ConstByteArray const &address)
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

