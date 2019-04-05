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
                                           ResourceSet const &resources,
                                           ResourceSet const &raw_resources)
  : StateAdapter(storage, scope)
{
  enable_writes_ = true;

  // Populate the allowed resources
  for (auto const &hash : raw_resources)
  {
    allowed_accesses_.insert(std::string{hash});
#ifndef NDEBUG
    FETCH_LOG_INFO(LOGGING_NAME, "Pushing allowed (raw): ", std::string{hash});
#endif
  }

  for (auto const &key : resources)
  {
    std::string full = std::string{scope.full_name()} + ".state." + std::string{key};
    allowed_accesses_.insert(full);
#ifndef NDEBUG
    FETCH_LOG_INFO(LOGGING_NAME, "Pushing allowed: ", full);
#endif
  }

  // Lock resources
  for (auto const &full_resource : allowed_accesses_)
  {
    storage_.Lock(CreateAddress(full_resource));
  }
}

StateSentinelAdapter::~StateSentinelAdapter()
{
  for (auto const &full_resource : allowed_accesses_)
  {
    storage_.Unlock(CreateAddress(full_resource));
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

  return StateAdapter::Read(key, data, size);
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

  return StateAdapter::Write(key, data, size);
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
  bool result = allowed_accesses_.find(key) != allowed_accesses_.end();

#ifndef NDEBUG
  if (!result)
  {
    std::ostringstream all_resources;

    for (auto const &access : allowed_accesses_)
    {
      all_resources << access << std::endl;
    }

    FETCH_LOG_WARN(LOGGING_NAME, "Failed to access resource \n", key, "\nAll resources: \n",
                   all_resources.str());
    // exit(1);
  }
#endif

  return result;
}

}  // namespace ledger
}  // namespace fetch
