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

#include "beacon/beacon_complaints_manager.hpp"

#include <cstdint>
#include <mutex>

namespace fetch {
namespace beacon {

constexpr char const *LOGGING_NAME = "DKGComplaints";

void ComplaintsManager::ResetCabinet(MuddleAddress const &address, uint32_t qual_size,
                                     uint32_t threshold)
{
  FETCH_LOCK(mutex_);
  qual_size_ = qual_size;
  threshold_ = threshold;
  address_   = address;
  finished_  = false;
  complaints_counter_.clear();
  complaints_from_.clear();
  complaints_.clear();
  complaints_received_.clear();
}

void ComplaintsManager::AddComplaintAgainst(MuddleAddress const &complaint_address)
{
  FETCH_LOCK(mutex_);
  complaints_counter_[complaint_address].insert(address_);
}

void ComplaintsManager::AddComplaintsFrom(ComplaintsMessage const &msg,
                                          MuddleAddress const &    from_id)
{
  FETCH_LOCK(mutex_);
  // Check if we have received a complaints message from this node before and if not log that we
  // received a complaint message
  if (complaints_received_.find(from_id) == complaints_received_.end())
  {
    complaints_received_.insert(from_id);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Duplicate complaints received. Discarding.");
    return;
  }

  for (auto const &bad_node : msg.complaints())
  {
    complaints_counter_[bad_node].insert(from_id);
    // If a node receives complaint against itself then store in complaints from
    // for answering later
    if (bad_node == address_)
    {
      complaints_from_.insert(from_id);
    }
  }
}

bool ComplaintsManager::IsFinished(std::set<Identity> const &cabinet)
{
  FETCH_LOCK(mutex_);
  if (!finished_ && complaints_received_.size() >= qual_size_ - 1)
  {
    // Add miners which did not send a complaint to complaints
    for (auto const &cab : cabinet)
    {
      if (cab.identifier() != address_ &&
          complaints_received_.find(cab.identifier()) == complaints_received_.end())
      {
        complaints_.insert(cab.identifier());
      }
    }
    // All miners who have received threshold or more complaints are also disqualified
    for (auto const &node_complaints : complaints_counter_)
    {
      if (node_complaints.second.size() >= threshold_)
      {
        complaints_.insert(node_complaints.first);
      }
    }
    finished_ = true;
  }
  return finished_;
}

std::set<ComplaintsManager::MuddleAddress> ComplaintsManager::ComplaintsAgainstSelf() const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  return complaints_from_;
}

bool ComplaintsManager::FindComplaint(MuddleAddress const &complaint_address,
                                      MuddleAddress const &complainer_address) const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  auto iter = complaints_counter_.find(complaint_address);
  if (iter == complaints_counter_.end())
  {
    return false;
  }
  return (complaints_counter_.at(complaint_address).find(complainer_address) !=
          complaints_counter_.at(complaint_address).end());
}

uint32_t ComplaintsManager::ComplaintsCount(MuddleAddress const &address) const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  auto iter = complaints_counter_.find(address);
  if (iter == complaints_counter_.end())
  {
    return 0;
  }
  return static_cast<uint32_t>(complaints_counter_.at(address).size());
}

std::set<dkg::ComplaintsMessage::MuddleAddress> ComplaintsManager::Complaints() const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  return complaints_;
}

void ComplaintAnswersManager::Init(std::set<MuddleAddress> const &complaints)
{
  FETCH_LOCK(mutex_);
  std::copy(complaints.begin(), complaints.end(), std::inserter(complaints_, complaints_.begin()));
}

void ComplaintAnswersManager::ResetCabinet(uint32_t qual_size)
{
  FETCH_LOCK(mutex_);
  qual_size_ = qual_size;
  finished_  = false;
  complaints_.clear();
}

void ComplaintAnswersManager::AddComplaintAgainst(MuddleAddress const &member)
{
  FETCH_LOCK(mutex_);
  complaints_.insert(member);
}

void ComplaintAnswersManager::AddComplaintAnswerFrom(MuddleAddress const &from,
                                                     Answer const &       complaint_answer)
{
  FETCH_LOCK(mutex_);
  if (complaint_answers_received_.find(from) != complaint_answers_received_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Duplicate complaint answer received");
    return;
  }
  complaint_answers_received_.insert({from, complaint_answer});
}

bool ComplaintAnswersManager::IsFinished(std::set<Identity> const &cabinet, Identity const &node_id)
{
  FETCH_LOCK(mutex_);
  if (!finished_ && complaint_answers_received_.size() >= qual_size_ - 1)
  {
    // Add miners which did not send a complaint answer to complaints
    for (auto const cab : cabinet)
    {
      if (cab == node_id)
      {
        continue;
      }
      if (complaint_answers_received_.find(cab.identifier()) == complaint_answers_received_.end())
      {
        complaints_.insert(cab.identifier());
      }
    }
    finished_ = true;
  }
  return finished_;
}

ComplaintAnswersManager::ComplaintAnswers ComplaintAnswersManager::ComplaintAnswersReceived() const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  return complaint_answers_received_;
}

std::set<ComplaintAnswersManager::MuddleAddress> ComplaintAnswersManager::BuildQual(
    std::set<MuddleAddress> const &cabinet) const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  std::set<MuddleAddress> qual;
  std::set_difference(cabinet.begin(), cabinet.end(), complaints_.begin(), complaints_.end(),
                      std::inserter(qual, qual.begin()));
  return qual;
}

void QualComplaintsManager::Reset()
{
  FETCH_LOCK(mutex_);
  finished_ = false;
  complaints_.clear();
  complaints_received_.clear();
}

void QualComplaintsManager::AddComplaintAgainst(MuddleAddress const &id)
{
  FETCH_LOCK(mutex_);
  complaints_.insert(id);
}

std::set<QualComplaintsManager::MuddleAddress> QualComplaintsManager::Complaints() const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  return complaints_;
}

void QualComplaintsManager::AddComplaintsFrom(
    MuddleAddress const &id, std::unordered_map<MuddleAddress, ExposedShares> const &complaints)
{
  FETCH_LOCK(mutex_);
  if (complaints_received_.find(id) != complaints_received_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Duplicate qual complaints received. Discarding.");
    return;
  }
  complaints_received_.insert({id, complaints});
}

bool QualComplaintsManager::IsFinished(std::set<MuddleAddress> const &qual,
                                       MuddleAddress const &node_id, uint32_t polynomial_degree)
{
  FETCH_LOCK(mutex_);
  if (!finished_)
  {
    uint32_t total_intersecting_complaints = 0;

    for (auto const &qualified_member : qual)
    {
      if (qualified_member != node_id &&
          complaints_received_.find(qualified_member) != complaints_received_.end())
      {
        total_intersecting_complaints++;
      }
    }

    if (total_intersecting_complaints >= polynomial_degree)
    {
      finished_ = true;
    }
  }
  return finished_;
}

const QualComplaintsManager::QualComplaints &QualComplaintsManager::ComplaintsReceived() const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  return complaints_received_;
}

std::size_t QualComplaintsManager::ComplaintsSize() const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  return complaints_.size();
}

bool QualComplaintsManager::FindComplaint(MuddleAddress const &id) const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  return complaints_.find(id) != complaints_.end();
}

}  // namespace beacon
}  // namespace fetch
