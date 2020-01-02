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
#include "core/byte_array/encoders.hpp"
#include "core/serializers/group_definitions.hpp"
#include "crypto/fnv.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "storage/resource_mapper.hpp"

#include <limits>
#include <type_traits>
#include <utility>

namespace fetch {
namespace storage {

/**
 * Constructs a Resource ID from an input hashed array
 *
 * @param id The hashed array
 */
ResourceID::ResourceID(byte_array::ConstByteArray id)
  : id_(std::move(id))
{
  assert(id.size() == RESOURCE_ID_SIZE_IN_BYTES);
}

/**
 * Gets the current id (hashed) value
 *
 * @return The id value
 */
byte_array::ConstByteArray ResourceID::id() const
{
  return id_;
}

/**
 * Gets the resource group value.
 *
 * @return The resource group value
 */
ResourceID::Group ResourceID::resource_group() const
{
  static_assert(std::is_integral<Group>::value, "Group type must be integer");
  assert(id_.size() >= sizeof(Group));

  return *reinterpret_cast<Group const *>(id_.pointer());
}

/**
 * Translates the resource group value into a lane index, given a specified
 * number of lanes
 *
 * @param log2_num_lanes The log2 number of lanes, i.e. for 4 lanes this would
 * be 2
 * @return The lane index
 */
ResourceID::Group ResourceID::lane(std::size_t log2_num_lanes) const
{
  // define the group mask
  Group const group_mask = (1u << log2_num_lanes) - 1u;

  return resource_group() & group_mask;
}

bool ResourceID::operator==(ResourceID const &other) const
{
  return id_ == other.id_;
}

bool ResourceID::operator<(ResourceID const &other) const
{
  return id_ < other.id_;
}

std::string ResourceID::ToString() const
{
  return static_cast<std::string>(ToBase64(id_));
}

ResourceAddress::ResourceAddress(byte_array::ConstByteArray const &address)
  : ResourceID(crypto::Hash<crypto::SHA256>(address))
  , address_{address}
{}

ResourceAddress::ResourceAddress(ResourceID const &rid)
  : ResourceID(rid)
{}

byte_array::ConstByteArray ResourceAddress::address() const
{
  return address_;
}

ResourceID const &ResourceAddress::as_resource_id() const
{
  return *this;
}

bool ResourceAddress::operator<(ResourceAddress const &other) const
{
  return address_ < other.address_;
}

bool ResourceAddress::operator==(ResourceAddress const &other) const
{
  return address_ == other.address_;
}

}  // namespace storage
}  // namespace fetch
