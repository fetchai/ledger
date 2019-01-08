#pragma once
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

#include "network/muddle/packet.hpp"

#include <set>

namespace fetch {
namespace muddle {

class Blacklist
{
  using Address    = Packet::Address;  // == a crypto::Identity.identifier_
  using RawAddress = Packet::RawAddress;
  using Mutex      = fetch::mutex::Mutex;
  using Lock       = std::unique_lock<Mutex>;
  using Contents   = std::set<Address>;

public:
  Blacklist()  = default;
  ~Blacklist() = default;

  void Add(const Address &address)
  {
    FETCH_LOCK(mutex_);
    contents_.insert(address);
  }

  void Remove(const Address &address)
  {
    FETCH_LOCK(mutex_);
    contents_.erase(address);
  }

  bool Contains(const Address &address) const
  {
    FETCH_LOCK(mutex_);
    return contents_.find(address) != contents_.end();
  }
  bool Contains(const RawAddress &raw_address) const
  {
    Packet::Address       address;
    byte_array::ByteArray buffer;
    buffer.Resize(raw_address.size());
    std::memcpy(buffer.pointer(), raw_address.data(), raw_address.size());
    address = buffer;

    FETCH_LOCK(mutex_);
    return contents_.find(address) != contents_.end();
  }

private:
  mutable Mutex mutex_{__LINE__, __FILE__};
  Contents      contents_;
};

}  // namespace muddle
}  // namespace fetch
