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

#include "ledger/chaincode/database_interface.hpp"

namespace fetch {
namespace ledger {

DatabaseInterface::DatabaseInterface(SmartContract *context) : context_{context} {}

DatabaseInterface::~DatabaseInterface() = default;

void DatabaseInterface::Allow(ByteArray const &resource)
{
  allowed_resources_.insert(resource);
}

bool DatabaseInterface::write(uint8_t const * const source, uint64_t dest_size, uint8_t const * const keyy, uint64_t key_size)
{
  ByteArray key{keyy, key_size};
  ByteArray value{source, dest_size};

  if(!AccessResource(key))
  {
    return false;
  }

  cached_resources_[key] = value;

  return true;
}

bool DatabaseInterface::read(uint8_t *dest, uint64_t dest_size, uint8_t const * const keyy, uint64_t key_size)
{
  ByteArray key{keyy, key_size};

  if(!AccessResource(key))
  {
    return false;
  }

  // Attempt to read from cache
  if(cached_resources_.find(key) != cached_resources_.end())
  {
    ByteArray value = cached_resources_[key];
    assert(value.size() == dest_size);

    memcpy(dest, value.pointer(), dest_size);

    return true;
  }

  // Read from state db, otherwise return zeroed memory
  ByteArray data;
  if(!context_->Get(data, key))
  {
    data.Resize(dest_size);
  }

  memcpy(dest, data.pointer(), dest_size);

  FETCH_LOG_WARN(LOGGING_NAME, "Reading from state: ", ToHex(key), " ", ToHex(data), " size: ", dest_size);

  cached_resources_[key] = data;

  return true;
}

bool DatabaseInterface::AccessResource(ByteArray const &key)
{
  if(allowed_resources_.find(key) == allowed_resources_.end())
  {
    std::ostringstream oss;

    oss <<"Transaction failed to access resource: " << ToHex(key) << " AKA "  << std::string{key}<< std::endl;
    oss <<"Allowed resources: " << std::endl;

    for(auto const &resource : allowed_resources_)
    {
      oss << ToHex(resource) << std::endl;
    }

    FETCH_LOG_WARN(LOGGING_NAME, oss.str());

    return false;
  }

  return true;
}

void DatabaseInterface::WriteBackToState()
{
  for(auto const &cached_item : cached_resources_)
  {
    auto address = cached_item.first;
    auto data = cached_item.second;

    FETCH_LOG_WARN(LOGGING_NAME, "Writing back to state: ", ToHex(address), " ", ToHex(data));

    context_->Set(data, address);
  }
}

}  // namespace ledger
}  // namespace fetch

