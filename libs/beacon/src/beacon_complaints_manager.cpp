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

void ComplaintsManager::ResetCabinet(MuddleAddress const &address, uint32_t threshold)
{
  FETCH_LOCK(mutex_);
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

void ComplaintsManager::AddComplaintsFrom(MuddleAddress const &                    from,
                                          std::unordered_set<MuddleAddress> const &complaints,
                                          std::set<MuddleAddress> const &          committee)
{
  FETCH_LOCK(mutex_);
  // Check if we have received a complaints message from this node before and if not log that we
  // received a complaint message
  if (complaints_received_.find(from) == complaints_received_.end())
  {
    complaints_received_.insert(from);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Duplicate complaints received. Discarding.");
    return;
  }

  for (auto const &bad_node : complaints)
  {
    if (committee.find(bad_node) != committee.end())
    {
      complaints_counter_[bad_node].insert(from);
      // If a node receives complaint against itself then store in complaints from
      // for answering later
      if (bad_node == address_)
      {
        complaints_from_.insert(from);
      }
    }
  }
}

void ComplaintsManager::Finish(std::set<MuddleAddress> const &cabinet)
{
  FETCH_LOCK(mutex_);
  if (!finished_)
  {
    // Add miners which did not send a complaint to complaints
    for (auto const &cab : cabinet)
    {
      if (cab != address_ && complaints_received_.find(cab) == complaints_received_.end())
      {
        complaints_.insert(cab);
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
}

uint32_t ComplaintsManager::NumComplaintsReceived() const
{
  FETCH_LOCK(mutex_);
  return static_cast<uint32_t>(complaints_received_.size());
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

void ComplaintAnswersManager::ResetCabinet()
{
  FETCH_LOCK(mutex_);
  finished_ = false;
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

void ComplaintAnswersManager::Finish(std::set<MuddleAddress> const &cabinet,
                                     MuddleAddress const &          node_id)
{
  FETCH_LOCK(mutex_);
  if (!finished_)
  {
    // Add miners which did not send a complaint answer to complaints
    for (auto const &cab : cabinet)
    {
      if (cab == node_id)
      {
        continue;
      }
      if (complaint_answers_received_.find(cab) == complaint_answers_received_.end())
      {
        complaints_.insert(cab);
      }
    }
    finished_ = true;
  }
}

uint32_t ComplaintAnswersManager::NumComplaintAnswersReceived() const
{
  FETCH_LOCK(mutex_);
  return static_cast<uint32_t>(complaint_answers_received_.size());
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

void QualComplaintsManager::Finish(std::set<MuddleAddress> const &qual,
                                   MuddleAddress const &          node_id)
{
  FETCH_LOCK(mutex_);
  if (!finished_)
  {
    for (auto const &qualified_member : qual)
    {
      if (qualified_member != node_id &&
          complaints_received_.find(qualified_member) == complaints_received_.end())
      {
        complaints_.insert(qualified_member);
      }
    }
    finished_ = true;
  }
}

uint32_t QualComplaintsManager::NumComplaintsReceived(std::set<MuddleAddress> const &qual) const
{
  FETCH_LOCK(mutex_);
  uint32_t qual_complaints{0};
  for (auto const &mem : qual)
  {
    if (complaints_received_.find(mem) != complaints_received_.end())
    {
      qual_complaints++;
    }
  }
  return qual_complaints;
}

QualComplaintsManager::QualComplaints QualComplaintsManager::ComplaintsReceived(
    std::set<MuddleAddress> const &qual) const
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  QualComplaints qual_complaints;
  for (auto const &mem : qual)
  {
    if (complaints_received_.find(mem) != complaints_received_.end())
    {
      qual_complaints.insert({mem, complaints_received_.at(mem)});
    }
  }
  return qual_complaints;
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
