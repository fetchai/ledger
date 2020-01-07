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

#include "core/byte_array/encoders.hpp"
#include "ledger/state_adapter.hpp"
#include "logging/logging.hpp"
#include "storage/resource_mapper.hpp"

using fetch::storage::ResourceAddress;
using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace ledger {

namespace {
constexpr char const *LOGGING_NAME = "StateAdapter";
}

/**
 * Constructs a state adapter from a storage interface and a scope
 *
 * @param storage The reference to the storage engine
 * @param scope The reference to the scope
 * @param allow_writes Enables write functionality
 */
StateAdapter::StateAdapter(StorageInterface &storage, ConstByteArray scope)
  : StateAdapter(storage, std::move(scope), Mode::READ_ONLY)
{}

StateAdapter::StateAdapter(StorageInterface &storage, ConstByteArray scope, Mode mode)
  : storage_{storage}
  , scope_{std::move(scope)}
  , mode_{mode}
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
  FETCH_LOG_DEBUG(LOGGING_NAME, "Read: ", key);

  Status status{Status::ERROR};

  // make the request to the storage engine
  auto const result = storage_.Get(CreateAddress(CurrentScope(), key));

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
  FETCH_LOG_DEBUG(LOGGING_NAME, "Write: ", key, " size: ", size);

  // early exit if we do not have permission to write to the storage interface
  if (Mode::READ_WRITE != mode_)
  {
    return Status::PERMISSION_DENIED;
  }

  auto write_val = ConstByteArray{reinterpret_cast<uint8_t const *>(data), size};

  // set the value on the storage engine
  storage_.Set(CreateAddress(CurrentScope(), key), write_val);

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
  // request the result
  auto const result = storage_.Get(CreateAddress(CurrentScope(), key));

  if (result.failed)
  {
    return Status::ERROR;
  }

  return Status::OK;
}

/**
 * Creates a scoped address from a string based key
 *
 * @param scope The contract context for the state variable
 * @param key The input key to be converted
 * @return The generated resource address for this key
 */
ResourceAddress StateAdapter::CreateAddress(ConstByteArray const &scope, ConstByteArray const &key)
{
  FETCH_LOG_DEBUG("StateAdapter", "Creating address for key: ", key, " scope: ", scope);

  return ResourceAddress{scope + ".state." + key};
}

void StateAdapter::PushContext(byte_array::ConstByteArray const &scope)
{
  scope_.emplace_back(scope);
}

void StateAdapter::PopContext()
{
  scope_.pop_back();
}

StateAdapter::ConstByteArray StateAdapter::CurrentScope() const
{
  return scope_.back();
}

}  // namespace ledger
}  // namespace fetch
