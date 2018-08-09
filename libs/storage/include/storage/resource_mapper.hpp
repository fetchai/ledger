#pragma once

#include "core/assert.hpp"
#include "core/byte_array/encoders.hpp"
#include "crypto/fnv.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

#include <limits>
#include <type_traits>

namespace fetch {
namespace storage {

/**
 * The Resource ID is a wrapper around the byte array. The implication is that a Resource ID is
 * designed to be the hashed version of a `ResourceAddress`
 */
class ResourceID
{
public:
  using Group = uint32_t;

  // Construction
  ResourceID() = default;
  explicit ResourceID(byte_array::ConstByteArray const &id);

  // Accessors
  byte_array::ConstByteArray id() const;
  Group                      resource_group() const;
  Group                      lane(std::size_t log2_num_lanes) const;

private:
  byte_array::ConstByteArray id_;  ///< The byte array containing the hashed resource address

  template <typename T>
  friend inline void Serialize(T &, ResourceID const &);
  template <typename T>
  friend inline void Deserialize(T &, ResourceID &);
};

/**
 * Constructs a Resource ID from an input hashed array
 *
 * @param id The hashed array
 */
inline ResourceID::ResourceID(byte_array::ConstByteArray const &id) : id_(id) {}

/**
 * Gets the current id (hashed) value
 *
 * @return The id value
 */
inline byte_array::ConstByteArray ResourceID::id() const { return id_; }

/**
 * Gets the resource group value.
 *
 * @return THe resource group value
 */
inline ResourceID::Group ResourceID::resource_group() const
{
  static_assert(std::is_integral<Group>::value, "Group type must be integer");
  assert(id_.size() > sizeof(Group));

  return *reinterpret_cast<Group const *>(id_.pointer());
}

/**
 * Translates the resource group value into a lane index, given a specified number of lanes
 *
 * @param log2_num_lanes The log2 number of lanes, i.e. for 4 lanes this would be 2
 * @return The lane index
 */
inline ResourceID::Group ResourceID::lane(std::size_t log2_num_lanes) const
{
  // define the group mask
  Group const group_mask = (1u << log2_num_lanes) - 1u;

  return resource_group() & group_mask;
}

/**
 * Serializes a specified `ResourceID` object with the given serializer
 *
 * @tparam T The type of the serializer
 * @param serializer The reference to the serializer
 * @param b The reference to the resource id
 */
template <typename T>
void Serialize(T &serializer, ResourceID const &b)
{
  serializer << b.id_;
}

/**
 * Deserializes a given `ResourceID` object from the specified serializer
 *
 * @tparam T The type of the serializer
 * @param serializer The reference to the serializer
 * @param b The reference to the output resource id
 */
template <typename T>
void Deserialize(T &serializer, ResourceID &b)
{
  serializer >> b.id_;
}

/**
 * The Resource Address is
 */
class ResourceAddress : public ResourceID
{
public:
  explicit ResourceAddress(byte_array::ConstByteArray const &address)
    : ResourceID(crypto::Hash<crypto::SHA256>(address))
  {
    address_ = address;
  }

  /**
   * Gets the canonical resources address
   *
   * @return The byte array containing the address
   */
  byte_array::ConstByteArray address() const { return address_; }

  /**
   * Helper method to down cast this object as a resource ID
   *
   * @return The reference to the resource id of this instance
   */
  ResourceID const &as_resource_id() const { return *this; }

private:
  byte_array::ByteArray address_;  ///< The canonical resource address
};

}  // namespace storage
}  // namespace fetch
