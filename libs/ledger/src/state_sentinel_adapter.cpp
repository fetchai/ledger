////------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "ledger/state_sentinel_adapter.hpp"
#include "storage/resource_mapper.hpp"

namespace fetch {
namespace ledger {

/**
 * Constructs a state adapter from a storage interface and a scope
 *
 * @param storage The reference to the storage engine
 * @param scope The reference to the scope
 */
StateSentinelAdapter::StateSentinelAdapter(StorageInterface &storage, Identifier scope,
                                           ShardIndex log2_num_shards, ShardIndexSet shards)
  : StateAdapter(storage, std::move(scope), true)
  , log2_num_shards_{log2_num_shards}
  , shards_{std::move(shards)}
{
  // Lock resources
  for (auto const &index : shards_)
  {
    storage_.Lock(index);
  }
}

StateSentinelAdapter::~StateSentinelAdapter()
{
  // Unlock resources
  for (auto const &index : shards_)
  {
    storage_.Unlock(index);
  }
}

/**
 * Read a value from the state store
 *
 * @param key The key to be accessed
 * @param data The pointer to the output buffer to be populated
 * @param size The size of the output buffer, if successful the size of the data will be written
 * back
 * @return OK if the read was successful, PERMISSION_DENIED if the key is incorrect, otherwise ERROR
 */
StateSentinelAdapter::Status StateSentinelAdapter::Read(std::string const &key, void *data,
                                                        uint64_t &size)
{
  if (!IsAllowedResource(WrapKeyWithScope(key)))
  {
    return Status::PERMISSION_DENIED;
  }

  // proxy the call the the state adapter
  auto const status = StateAdapter::Read(key, data, size);

  // update the counters
  if (Status::OK == status)
  {
    bytes_read_ += size;
  }

  ++lookups_;

  return status;
}

/**
 * Write a value to the state store
 *
 * @param key The key to be accessed
 * @param data The pointer to the input buffer for the data
 * @param size The size in bytes of the input buffer
 * @return OK if the write was successful, PERMISSION_DENIED if the key is incorrect, otherwise
 * ERROR
 */
StateSentinelAdapter::Status StateSentinelAdapter::Write(std::string const &key, void const *data,
                                                         uint64_t size)
{

  if (!IsAllowedResource(WrapKeyWithScope(key)))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to write to resource: ", WrapKeyWithScope(key));
    return Status::PERMISSION_DENIED;
  }

  // proxy call to the state adapter
  auto const status = StateAdapter::Write(key, data, size);

  // update the counters
  if (Status::OK == status)
  {
    bytes_written_ += size;
  }

  ++lookups_;

  return status;
}

/**
 * Checks to see if the specified key exists in the database
 *
 * @param key The key to be checked
 * @return OK if the key exists, PERMISSION_DENIED if the key is incorrect, ERROR is the key does
 * not exist
 */
StateSentinelAdapter::Status StateSentinelAdapter::Exists(std::string const &key)
{
  if (!IsAllowedResource(WrapKeyWithScope(key)))
  {
    return Status::PERMISSION_DENIED;
  }

  ++lookups_;

  return StateAdapter::Exists(key);
}

/**
 * Check whether the resource being requested is allowed
 *
 * @param: key key to check
 *
 * @return: whether it is allowed
 */
bool StateSentinelAdapter::IsAllowedResource(std::string const &key) const
{
  // build the associated resources address
  ResourceAddress const address{key};

  // determine which shard this resource is mapped to
  auto const mapped_shard = address.lane(log2_num_shards_);

  // calculate if this shard is in the allowed shard list
  bool const is_allowed = shards_.find(mapped_shard) != shards_.end();

#ifndef NDEBUG
  if (!is_allowed)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to access resource: ", key);
  }
#endif

  return is_allowed;
}

}  // namespace ledger
}  // namespace fetch
