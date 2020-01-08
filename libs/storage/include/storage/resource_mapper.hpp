#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/assert.hpp"
#include "core/serializers/group_definitions.hpp"
#include "crypto/fnv.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

#include <type_traits>
#include <utility>

namespace fetch {
namespace storage {

/**
 * The Resource ID is a wrapper around the byte array. The implication is that a
 * Resource ID is
 * designed to be the hashed version of a `ResourceAddress`
 */
class ResourceID
{
public:
  using Group = uint32_t;

  // Construction
  ResourceID() = default;
  explicit ResourceID(byte_array::ConstByteArray id);

  // Accessors
  byte_array::ConstByteArray id() const;
  Group                      resource_group() const;
  Group                      lane(std::size_t log2_num_lanes) const;

  bool operator==(ResourceID const &other) const;

  bool operator<(ResourceID const &other) const;

  std::string ToString() const;

  static constexpr std::size_t RESOURCE_ID_SIZE_IN_BITS  = 256;
  static constexpr std::size_t RESOURCE_ID_SIZE_IN_BYTES = RESOURCE_ID_SIZE_IN_BITS / 8;

private:
  byte_array::ConstByteArray id_;  ///< The byte array containing the hashed resource address

  template <typename T, typename D>
  friend struct serializers::ForwardSerializer;
};

/**
 * The Resource Address is
 */
class ResourceAddress : public ResourceID
{
public:
  explicit ResourceAddress(byte_array::ConstByteArray const &address);
  explicit ResourceAddress(ResourceID const &rid);

  ResourceAddress() = default;

  /**
   * Gets the canonical resources address
   *
   * @return The byte array containing the address
   */
  byte_array::ConstByteArray address() const;

  /**
   * Helper method to down cast this object as a resource ID
   *
   * @return The reference to the resource id of this instance
   */
  ResourceID const &as_resource_id() const;

  bool operator<(ResourceAddress const &other) const;

  bool operator==(ResourceAddress const &other) const;

private:
  byte_array::ConstByteArray address_;  ///< The canonical resource address
};

}  // namespace storage

namespace serializers {

template <typename D>
struct ForwardSerializer<storage::ResourceID, D>
{
public:
  using Type       = storage::ResourceID;
  using DriverType = D;

  template <typename Serializer>
  static void Serialize(Serializer &s, Type const &b)
  {
    s << b.id_;
  }

  template <typename Serializer>
  static void Deserialize(Serializer &s, Type &b)
  {
    s >> b.id_;
  }
};

}  // namespace serializers
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::storage::ResourceID>
{
  std::size_t operator()(fetch::storage::ResourceID const &rid) const
  {
    auto const &id = rid.id();
    assert(id.size() >= sizeof(std::size_t));

    // this is generally fine because the resource ID is in fact a SHA256
    return *reinterpret_cast<std::size_t const *>(id.pointer());
  }
};

template <>
struct hash<fetch::storage::ResourceAddress>
{
  std::size_t operator()(fetch::storage::ResourceID const &rid) const
  {
    auto const &id = rid.id();
    assert(id.size() >= sizeof(std::size_t));

    // this should be fine since the ResourceAddress is actually a SHA256
    return *reinterpret_cast<std::size_t const *>(id.pointer());
  }
};

}  // namespace std
