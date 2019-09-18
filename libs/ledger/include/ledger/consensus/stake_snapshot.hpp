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

#include "crypto/identity.hpp"

#include <memory>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * Object for keeping track of the current identities which have stakes. Also facilitates the
 * selection of identities based on an entropy source.
 *
 * Conceptually this object represents a stake information for a single point of time, however, in
 * general the stake snapshots will be reused for the entire period of a stake period.
 */
class StakeSnapshot
{
public:
  using Identity     = crypto::Identity;
  using Committee    = std::vector<Identity>;
  using CommitteePtr = std::shared_ptr<Committee>;

  static constexpr char const *LOGGING_NAME = "StakeSnapshot";

  // Construction / Destruction
  StakeSnapshot()                      = default;
  StakeSnapshot(StakeSnapshot const &) = default;
  StakeSnapshot(StakeSnapshot &&)      = default;
  ~StakeSnapshot()                     = default;

  CommitteePtr BuildCommittee(uint64_t entropy, std::size_t count) const;

  /// @name Stake Updates
  /// @{
  uint64_t LookupStake(Identity const &identity) const;
  void     UpdateStake(Identity const &identity, uint64_t stake);
  /// @}

  /// @name Basic Accessors
  /// @{
  uint64_t    total_stake() const;
  std::size_t size() const;
  /// @}

  template <typename Functor>
  void IterateOver(Functor &&functor) const;

  // Operators
  StakeSnapshot &operator=(StakeSnapshot const &) = default;
  StakeSnapshot &operator=(StakeSnapshot &&) = default;

private:
  struct Record
  {
    Identity identity;
    uint64_t stake;
  };

  using RecordPtr     = std::shared_ptr<Record>;
  using IdentityIndex = std::unordered_map<Identity, RecordPtr>;
  using StakeIndex    = std::vector<RecordPtr>;

  IdentityIndex identity_index_{};  ///< Map of Identity to Record
  StakeIndex    stake_index_;       ///< Array of Records
  uint64_t      total_stake_{0};    ///< Total stake cache
};

/**
 * Get the total amount staked
 *
 * @return The value of the total amount staked
 */
inline uint64_t StakeSnapshot::total_stake() const
{
  return total_stake_;
}

/**
 * Get the number of identities that have staked
 *
 * @return The number of unique identities that have staked
 */
inline std::size_t StakeSnapshot::size() const
{
  return identity_index_.size();
}

/**
 * Iterate over the contents of the snapshot
 *
 * @tparam Functor The type of the functor
 * @param functor The reference to the functor
 */
template <typename Functor>
void StakeSnapshot::IterateOver(Functor &&functor) const
{
  for (auto const &element : identity_index_)
  {
    functor(element.first, element.second->stake);
  }
}

}  // namespace ledger
}  // namespace fetch
