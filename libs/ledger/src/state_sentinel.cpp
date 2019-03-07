//------------------------------------------------------------------------------
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

#include "ledger/state_sentinel.hpp"
#include "vm/string.hpp"

using fetch::storage::ResourceAddress;

namespace fetch {
namespace ledger {

/**
 * Constructs a state adapter from a storage interface and a scope
 *
 * @param storage The reference to the storage engine
 * @param scope The reference to the scope
 */
StateAdapter::StateAdapter(StorageInterface &storage, Identifier scope)
  : storage_{storage}
  , scope_{std::move(scope)}
{
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
StateAdapter::Status StateAdapter::Read(std::string const &key, void *data, uint64_t &size)
{
  Status status{Status::ERROR};

  // make the request to the storage engine
  auto const result = storage_.Get(CreateAddress(scope_, key));

  // ensure the check was not found
  if (!result.failed)
  {
    // ensure the buffer is the correct size
    if (size < result.document.size())
    {
      size   = result.document.size();
      status = Status::BUFFER_TOO_SMALL;
    }
    else  // normal case the buffer is fine
    {
      // copy the contents of the buffer into the output buffer
      result.document.ReadBytes(reinterpret_cast<uint8_t *>(data), result.document.size());

      // update the output size
      size = result.document.size();

      status = Status::OK;
    }
  }

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
StateAdapter::Status StateAdapter::Write(std::string const &key, void const *data, uint64_t size)
{
  FETCH_UNUSED(key);
  FETCH_UNUSED(data);
  FETCH_UNUSED(size);

  // this operation is not supported in this version of the adapter
  return Status::PERMISSION_DENIED;
}

/**
 * Checks to see if the specified key exists in the database
 *
 * @param key The key to be checked
 * @return OK if the key exists, PERMISSION_DENIED if the key is incorrect, ERROR is the key does
 * not exist
 */
StateAdapter::Status StateAdapter::Exists(std::string const &key)
{
  Status status{Status::ERROR};

  // request the result
  auto const result = storage_.Get(CreateAddress(scope_, key));

  if (!result.failed)
  {
    status = Status::OK;
  }

  return status;
}

/**
 * Creates a scoped address from a string based key
 *
 * @param scope The contract context for the state variable
 * @param key The input key to be converted
 * @return The generated resource address for this key
 */
ResourceAddress StateAdapter::CreateAddress(Identifier const &scope, ConstByteArray const &key)
{
  FETCH_LOG_DEBUG("StateAdapter", "Creating address for key: ", key.ToBase64(),
                  " scope: ", scope.full_name());
  return ResourceAddress{scope.full_name() + ".state." + key};
}

/**
 * Creates a read/write state adapter based on a specified storage engine and scope
 *
 * @param storage The underlying storage engine
 * @param scope The scope for all state
 */
StateSentinelAdapter::StateSentinelAdapter(StorageInterface &storage, Identifier scope, ResourceSet resources)
  : StateAdapter{storage, std::move(scope)}
  , resources_{std::move(resources)}
{
  // lock all the resources
  for (auto const &resource : resources_)
  {
    storage_.Lock(CreateAddress(scope_, resource));
  }
}

StateSentinelAdapter::~StateSentinelAdapter()
{
  // unlock all the resources
  for (auto const &resource : resources_)
  {
    storage_.Unlock(CreateAddress(scope_, resource));
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
  Status status{Status::PERMISSION_DENIED};

  // ensure the key is in the allowed set
  if (IsAllowedResource(key))
  {
    FETCH_LOG_DEBUG("StateSentinel", "Permission Accepted: ", ConstByteArray{key});

    // proxy the call to the parent class
    status = StateAdapter::Read(key, data, size);
  }
  else
  {
    FETCH_LOG_DEBUG("StateSentinel", "Permission Denied: ", ConstByteArray{key});
  }

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
  Status status{Status::PERMISSION_DENIED};

  // ensure the key is in the allowed set
  if (IsAllowedResource(key))
  {
    // set the value on the storage engine
    storage_.Set(CreateAddress(scope_, key),
                 ConstByteArray{reinterpret_cast<uint8_t const *>(data), size});
  }
  else
  {
    FETCH_LOG_WARN("Sentinel", "Unable to write to resource: ", key);
  }

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
  Status status{Status::PERMISSION_DENIED};

  // ensure the key is in the allowed set
  if (IsAllowedResource(key))
  {
    // proxy the call to the parent class
    status = StateAdapter::Exists(key);
  }

  return status;
}

/**
 * Internal: Determines if a specified key has been allowed
 *
 * @param key The key being requested
 * @return true if allowed, otherwise false
 */
bool StateSentinelAdapter::IsAllowedResource(std::string const &key) const
{
  return resources_.find(key) != resources_.end();
}

}  // namespace ledger
}  // namespace fetch
