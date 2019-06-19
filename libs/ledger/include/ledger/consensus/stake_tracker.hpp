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

#include "core/mutex.hpp"
#include "ledger/chain/address.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class StakeTracker
{
public:
  using AddressArray = std::vector<Address>;

  // Construction / Destruction
  StakeTracker()                     = default;
  StakeTracker(StakeTracker const &) = delete;
  StakeTracker(StakeTracker &&)      = delete;
  ~StakeTracker()                    = default;

  AddressArray Sample(uint64_t entropy, std::size_t count);

  /// @name Stake Updates
  /// @{
  uint64_t LookupStake(Address const &address) const;
  void     UpdateStake(Address const &address, uint64_t stake);
  /// @}

  /// @name Basic Accessors
  /// @{
  uint64_t total_stake() const;
  std::size_t size() const;
  /// @}

  // Operators
  StakeTracker &operator=(StakeTracker const &) = delete;
  StakeTracker &operator=(StakeTracker &&) = delete;

private:
  struct Record
  {
    Address  address;
    uint64_t stake;
  };

  using RecordPtr    = std::shared_ptr<Record>;
  using AddressIndex = std::unordered_map<Address, RecordPtr>;
  using StakeIndex   = std::vector<RecordPtr>;
  using Mutex        = mutex::Mutex;

  mutable Mutex lock_{__LINE__, __FILE__};
  AddressIndex  address_index_{};
  StakeIndex    stake_index_;
  uint64_t      total_stake_{0};
};

inline uint64_t StakeTracker::total_stake() const
{
  FETCH_LOCK(lock_);
  return total_stake_;
}

inline std::size_t StakeTracker::size() const
{
  FETCH_LOCK(lock_);
  return address_index_.size();
}


}  // namespace ledger
}  // namespace fetch
