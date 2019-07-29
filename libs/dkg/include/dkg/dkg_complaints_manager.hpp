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

#include "dkg/dkg_messages.hpp"

#include <atomic>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace dkg {

/**
 * Helper classes for managing complaint related messages in DKG
 */

/**
 * This class manages the complaint messages sent in the first part of the DKG involved
 * in constructing the qualified set, the set of cabinet members who can take part in the
 * threshold signing
 */
class ComplaintsManager
{
  using MuddleAddress = byte_array::ConstByteArray;

  uint32_t cabinet_size_{0};  ///< Size of cabinet
  std::unordered_map<MuddleAddress, uint32_t>
                          complaints_counter_;  ///< Counter for number complaints received by a cabinet member
  std::set<MuddleAddress> complaints_from_;  ///< Set of members who complaints against self
  std::set<MuddleAddress> complaints_;       ///< Set of members who we are complaining against
  std::atomic<uint32_t>   complaints_received_counter_{
      0};                                ///< Counter for number of complaint messages received
  std::vector<bool> complaints_received_;  ///< Vector which marks the true/false for whether we
                                           ///< have received a complaint message
                                           ///<  from each cabinet member by their index
  std::atomic<bool> finished_{
      false};  ///< Bool denoting whether we have collected complaint messages from everyone
  mutable std::mutex mutex_;

public:
  ComplaintsManager() = default;

  void ResetCabinet(uint32_t cabinet_size);
  void Count(MuddleAddress const &address);
  void Add(std::shared_ptr<ComplaintsMessage> const &msg_ptr, MuddleAddress const &from_id,
           uint32_t from_index, MuddleAddress node_address);
  bool IsFinished(std::set<MuddleAddress> const &miners, uint32_t node_index, uint32_t threshold);
  void Clear();

  std::set<MuddleAddress> ComplaintsFrom() const;
  std::set<MuddleAddress> Complaints() const;
};

/**
 * This class managers complaints at the second state of the DKG when the qualified set of
 * cabinet members, who passed the first round of complaints, have a round of complaints
 */
class QualComplaintsManager
{
  using MuddleAddress = byte_array::ConstByteArray;

  std::set<MuddleAddress> complaints_;           ///< Cabinet members we complain against
  std::set<MuddleAddress> complaints_received_;  ///< Set of cabinet members we have received a qual
                                                 ///< complaint message from
  mutable std::mutex mutex_;

public:
  QualComplaintsManager() = default;

  void        Complaints(MuddleAddress const &id);
  void        Received(MuddleAddress const &id);
  std::size_t ComplaintsSize() const;
  bool        ComplaintsFind(MuddleAddress const &id) const;
  bool        IsFinished(std::set<MuddleAddress> const &qual, MuddleAddress const &node_id);
  void        Clear();
};

/**
 * This class manages the complaint answer messages
 */
class ComplaintsAnswerManager
{
  using MuddleAddress = byte_array::ConstByteArray;

  uint32_t                cabinet_size_;
  std::set<MuddleAddress> complaints_;
  std::vector<bool>       complaint_answers_received_;
  std::atomic<uint32_t>   complaint_answers_received_counter_{0};
  std::atomic<bool>       finished_{false};
  std::mutex              mutex_;

public:
  ComplaintsAnswerManager() = default;

  void                    Init(std::set<MuddleAddress> const &complaints);
  void                    ResetCabinet(uint32_t cabinet_size);
  void                    Add(MuddleAddress const &miner);
  void                    Count(uint32_t from_index);
  bool                    IsFinished(std::set<MuddleAddress> const &miners, uint32_t index);
  std::set<MuddleAddress> BuildQual(std::set<MuddleAddress> const &miners);
  void                    Clear();
};

}  // namespace dkg
}  // namespace fetch
