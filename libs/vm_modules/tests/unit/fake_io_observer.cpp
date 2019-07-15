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

#include "fake_io_observer.hpp"

#include <cstdint>
#include <string>
#include <utility>

FakeIoObserver::Status FakeIoObserver::Read(std::string const &key, void *data, uint64_t &size)
{
  // check to see if the key is permitted
  if (!IsPermittedKey(key))
  {
    return Status::PERMISSION_DENIED;
  }

  // check to see if the key exists
  auto it = data_.find(key);
  if (it == data_.end())
  {
    return Status::ERROR;
  }

  // ensure the buffer is the correct size
  auto const &stored_data = it->second;
  auto const  orig_size{size};
  size = stored_data.size();
  if (orig_size < stored_data.size())
  {
    return Status::BUFFER_TOO_SMALL;
  }

  // copy the data back
  stored_data.ReadBytes(reinterpret_cast<uint8_t *>(data), size);

  return Status::OK;
}

FakeIoObserver::Status FakeIoObserver::Write(std::string const &key, void const *data,
                                             uint64_t size)
{
  ConstByteArray const value{reinterpret_cast<uint8_t const *>(data), size};

  // check to see if the key is permitted
  if (!IsPermittedKey(key))
  {
    return Status::PERMISSION_DENIED;
  }

  // store / update the data
  data_[key] = value;
  return Status::OK;
}

FakeIoObserver::Status FakeIoObserver::Exists(std::string const &key)
{
  return (data_.find(key) != data_.end()) ? Status::OK : Status::ERROR;
}

void FakeIoObserver::SetKeyValue(std::string const &key, ConstByteArray const &value)
{
  data_[key] = value;
}

void FakeIoObserver::SetDenied(std::string const &key)
{
  denied_keys_.insert(key);
}
