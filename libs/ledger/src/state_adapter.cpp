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

#include "ledger/state_adapter.hpp"
#include "storage/resource_mapper.hpp"

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
{}

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

  auto new_key = WrapKeyWithScope(key);

  // make the request to the storage engine
  auto const result = storage_.Get(CreateAddress(new_key));

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
  if (!enable_writes_)
  {
    return Status::OK;
  }

  auto new_key = WrapKeyWithScope(key);

  auto write_val = ConstByteArray{reinterpret_cast<uint8_t const *>(data), size};

  // set the value on the storage engine
  storage_.Set(CreateAddress(new_key), write_val);

  return Status::OK;
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

  auto new_key = WrapKeyWithScope(key);

  // request the result
  auto const result = storage_.Get(CreateAddress(new_key));

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
 * Creates a non-scoped address
 *
 * @param key The input key to be converted
 * @return The generated resource address for this key
 */
ResourceAddress StateAdapter::CreateAddress(ConstByteArray const &key)
{
  FETCH_LOG_DEBUG("StateAdapter", "Creating address for key: ", key.ToBase64(), " (no scope)");
  return ResourceAddress{key};
}

void StateAdapter::PushContext(Identifier const &scope)
{
  scope_.push_back(scope);
}

void StateAdapter::PopContext()
{
  scope_.pop_back();
}

std::string StateAdapter::WrapKeyWithScope(std::string const &key)
{
  return std::string{scope_.back().full_name() + ".state." + std::string{key}};
}

}  // namespace ledger
}  // namespace fetch
