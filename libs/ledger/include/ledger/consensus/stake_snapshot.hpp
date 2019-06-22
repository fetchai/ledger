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

/**
 * Object for keeping track of the current addresses which have stakes. Also facilitates the
 * selection of addresses based on an entropy source.
 *
 * Conceptually this object representats
 */
class StakeSnapshot
{
public:
  using AddressArray = std::vector<Address>;

  // Construction / Destruction
  StakeSnapshot()                      = default;
  StakeSnapshot(StakeSnapshot const &) = delete;
  StakeSnapshot(StakeSnapshot &&)      = delete;
  ~StakeSnapshot()                     = default;

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
  StakeSnapshot &operator=(StakeSnapshot const &) = delete;
  StakeSnapshot &operator=(StakeSnapshot &&) = delete;

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

  mutable Mutex lock_{__LINE__, __FILE__};  ///< Class level lock
  AddressIndex  address_index_{};           ///< Map of Address to Record
  StakeIndex    stake_index_;               ///< Array of Records
  uint64_t      total_stake_{0};            ///< Total stake cache
};

/**
 * Get the total amount staked
 *
 * @return The value of the total amount staked
 */
inline uint64_t StakeSnapshot::total_stake() const
{
  FETCH_LOCK(lock_);
  return total_stake_;
}

/**
 * Get the number of addresses that have staked
 *
 * @return The number of unique addresses that have staked
 */
inline std::size_t StakeSnapshot::size() const
{
  FETCH_LOCK(lock_);
  return address_index_.size();
}

}  // namespace ledger
}  // namespace fetch
