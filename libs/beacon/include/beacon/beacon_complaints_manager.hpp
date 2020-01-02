#pragma once
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

#include "dkg/dkg_messages.hpp"

#include <atomic>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace beacon {

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
  using MuddleAddress  = byte_array::ConstByteArray;
  using Cabinet        = std::set<MuddleAddress>;
  using ComplaintsList = std::set<MuddleAddress>;

  uint32_t      threshold_{0};  ///< DKG threshold
  MuddleAddress address_;       ///< Address of node
  std::unordered_map<MuddleAddress, std::unordered_set<MuddleAddress>>
                          complaints_counter_{};  ///< Counter for number complaints received by a cabinet member
  std::set<MuddleAddress> complaints_{};  ///< Set of members who we are complaining against
  std::unordered_map<MuddleAddress, ComplaintsList>
      complaints_received_{};  ///< Set of members whom we have received a complaint message from
                               ///< and their complaints
  bool finished_{
      false};  ///< Bool denoting whether we have collected complaint messages from everyone
  mutable Mutex mutex_;

public:
  ComplaintsManager() = default;

  void ResetCabinet(MuddleAddress const &address, uint32_t threshold);
  void AddComplaintAgainst(MuddleAddress const &complaint_address);
  void AddComplaintsFrom(MuddleAddress const &from, ComplaintsList const &complaints,
                         Cabinet const &cabinet);
  void Finish(Cabinet const &cabinet);

  uint32_t                          NumComplaintsReceived(Cabinet const &cabinet) const;
  std::unordered_set<MuddleAddress> ComplaintsAgainstSelf() const;
  bool                              FindComplaint(MuddleAddress const &complaint_address,
                                                  MuddleAddress const &complainer_address) const;
  ComplaintsList                    Complaints() const;
  uint32_t                          ComplaintsCount(MuddleAddress const &address) const;
};

/**
 * This class manages the complaint answer messages
 */
class ComplaintAnswersManager
{
  using MuddleAddress    = byte_array::ConstByteArray;
  using Share            = dkg::DKGMessage::Share;
  using ExposedShares    = std::pair<Share, Share>;
  using Answer           = std::unordered_map<MuddleAddress, ExposedShares>;
  using ComplaintAnswers = std::unordered_map<MuddleAddress, Answer>;
  using ComplaintsList   = std::set<MuddleAddress>;
  using Cabinet          = std::set<MuddleAddress>;

  ComplaintsList   complaints_;
  ComplaintAnswers complaint_answers_received_;
  bool             finished_{false};
  mutable Mutex    mutex_;

public:
  ComplaintAnswersManager() = default;

  void     Init(ComplaintsList const &complaints);
  void     ResetCabinet();
  void     AddComplaintAgainst(MuddleAddress const &member);
  void     AddComplaintAnswerFrom(MuddleAddress const &from, Answer const &complaint_answer);
  void     Finish(Cabinet const &cabinet, MuddleAddress const &node_id);
  uint32_t NumComplaintAnswersReceived(Cabinet const &cabinet) const;
  ComplaintAnswers ComplaintAnswersReceived() const;
  Cabinet          BuildQual(Cabinet const &cabinet) const;
};

/**
 * This class managers complaints at the second state of the DKG when the qualified set of
 * cabinet members, who passed the first round of complaints, have a round of complaints
 */
class QualComplaintsManager
{
  using MuddleAddress = byte_array::ConstByteArray;
  using Share         = dkg::DKGMessage::Share;
  using ExposedShares = std::pair<Share, Share>;
  using QualComplaints =
      std::unordered_map<MuddleAddress, std::unordered_map<MuddleAddress, ExposedShares>>;
  using ComplaintsList = std::set<MuddleAddress>;
  using Cabinet        = std::set<MuddleAddress>;

  bool           finished_{false};
  ComplaintsList complaints_;           ///< Cabinet members we complain against
  QualComplaints complaints_received_;  ///< Set of cabinet members we have received a qual
  ///< complaint message from
  mutable Mutex mutex_;

public:
  QualComplaintsManager() = default;

  void Reset();
  void AddComplaintAgainst(MuddleAddress const &id);
  void AddComplaintsFrom(MuddleAddress const &                                   id,
                         std::unordered_map<MuddleAddress, ExposedShares> const &complaints);
  void Finish(Cabinet const &qual, MuddleAddress const &node_id);

  uint32_t                                       NumComplaintsReceived(Cabinet const &qual) const;
  QualComplaints                                 ComplaintsReceived() const;
  std::size_t                                    ComplaintsSize() const;
  bool                                           FindComplaint(MuddleAddress const &id) const;
  std::set<QualComplaintsManager::MuddleAddress> Complaints() const;
};
}  // namespace beacon
}  // namespace fetch
