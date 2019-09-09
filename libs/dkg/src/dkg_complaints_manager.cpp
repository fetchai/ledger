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

#include "dkg/dkg_complaints_manager.hpp"

namespace fetch {
namespace dkg {

constexpr char const *LOGGING_NAME = "DKGComplaints";

void ComplaintsManager::ResetCabinet(uint32_t cabinet_size)
{
  FETCH_LOCK(mutex_);
  cabinet_size_ = cabinet_size;
  finished_     = false;
  complaints_counter_.clear();
  complaints_from_.clear();
  complaints_.clear();
  complaints_received_.clear();
}

void ComplaintsManager::Count(MuddleAddress const &address)
{
  std::lock_guard<std::mutex> lock(mutex_);
  ++complaints_counter_[address];
}

void ComplaintsManager::Add(ComplaintsMessage const &msg, MuddleAddress const &from_id,
                            MuddleAddress const &address)
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
    ++complaints_counter_[bad_node];
    // If a node receives complaint against itself then store in complaints from
    // for answering later
    if (bad_node == address)
    {
      complaints_from_.insert(from_id);
    }
  }
}

bool ComplaintsManager::IsFinished(uint32_t polynomial_degree)
{
  FETCH_LOCK(mutex_);
  if (complaints_received_.size() == cabinet_size_ - 1)
  {
    // TODO(jmw): Add miners which did not send a complaint to complaints?
    // All miners who have received over t complaints are also disqualified
    for (auto const &node_complaints : complaints_counter_)
    {
      if (node_complaints.second > polynomial_degree)
      {
        complaints_.insert(node_complaints.first);
      }
    }
    finished_ = true;
    return true;
  }
  return false;
}

std::set<ComplaintsMessage::MuddleAddress> ComplaintsManager::ComplaintsFrom() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  assert(finished_);
  return complaints_from_;
}

uint32_t ComplaintsManager::ComplaintsCount(MuddleAddress const &address)
{
  std::lock_guard<std::mutex> lock(mutex_);
  auto                        iter = complaints_counter_.find(address);
  if (iter == complaints_counter_.end())
  {
    return 0;
  }
  return complaints_counter_.at(address);
}

std::set<ComplaintsMessage::MuddleAddress> ComplaintsManager::Complaints() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  assert(finished_);
  return complaints_;
}

void ComplaintsManager::Clear()
{
  std::lock_guard<std::mutex> lock(mutex_);
  complaints_counter_.clear();
  complaints_from_.clear();
  complaints_.clear();
  complaints_received_.clear();
}

void QualComplaintsManager::Complaints(MuddleAddress const &id)
{
  std::lock_guard<std::mutex> lock(mutex_);
  complaints_.insert(id);
}

std::set<QualComplaintsManager::MuddleAddress> QualComplaintsManager::Complaints() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  // TODO(HUT): Discuss w/Jenny
  /*assert(finished_); */
  return complaints_;
}

void QualComplaintsManager::Received(MuddleAddress const &                               id,
                                     std::unordered_map<CabinetId, ExposedShares> const &complaints)
{
  std::lock_guard<std::mutex> lock(mutex_);
  complaints_received_.insert({id, complaints});
}

const QualComplaintsManager::QualComplaints &QualComplaintsManager::ComplaintsReceived() const
{
  return complaints_received_;
}

std::size_t QualComplaintsManager::ComplaintsSize() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return complaints_.size();
}

bool QualComplaintsManager::ComplaintsFind(MuddleAddress const &id) const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return complaints_.find(id) != complaints_.end();
}

bool QualComplaintsManager::IsFinished(std::set<MuddleAddress> const &qual,
                                       MuddleAddress const &node_id, uint32_t threshold)
{
  FETCH_LOCK(mutex_);

  uint32_t total_intersecting_complaints = 0;

  for (auto const &qualified_member : qual)
  {
    if (qualified_member != node_id &&
        complaints_received_.find(qualified_member) != complaints_received_.end())
    {
      total_intersecting_complaints++;
    }
  }

  return total_intersecting_complaints >= threshold;
}

void QualComplaintsManager::Clear()
{
  FETCH_LOCK(mutex_);
  complaints_.clear();
  complaints_received_.clear();
}

void QualComplaintsManager::Reset()
{
  FETCH_LOCK(mutex_);
  finished_ = false;
  complaints_.clear();
  complaints_received_.clear();
}

void ComplaintsAnswerManager::Init(std::set<MuddleAddress> const &complaints)
{
  FETCH_LOCK(mutex_);
  std::copy(complaints.begin(), complaints.end(), std::inserter(complaints_, complaints_.begin()));
}

void ComplaintsAnswerManager::ResetCabinet(uint32_t cabinet_size)
{
  FETCH_LOCK(mutex_);
  cabinet_size_ = cabinet_size;
  finished_     = false;
  complaints_.clear();
}

void ComplaintsAnswerManager::Add(MuddleAddress const &member)
{
  FETCH_LOCK(mutex_);
  complaints_.insert(member);
}

bool ComplaintsAnswerManager::Count(MuddleAddress const &from)
{
  FETCH_LOCK(mutex_);
  if (complaint_answers_received_.find(from) == complaint_answers_received_.end())
  {
    complaint_answers_received_.insert(from);
    return true;
  }
  else
  {
    return false;
  }
}

bool ComplaintsAnswerManager::IsFinished(uint32_t threshold)
{
  FETCH_LOCK(mutex_);

  if (complaint_answers_received_.size() >= threshold)
  {
    // TODO(jmw): Add miners which did not send a complaint answer to complaints to complaints?
    finished_ = true;
    return true;
  }
  return false;
}

std::set<ComplaintsAnswerManager::MuddleAddress> ComplaintsAnswerManager::BuildQual(
    std::set<MuddleAddress> const &cabinet)
{
  FETCH_LOCK(mutex_);
  assert(finished_);
  std::set<MuddleAddress> qual;
  std::set_difference(cabinet.begin(), cabinet.end(), complaints_.begin(), complaints_.end(),
                      std::inserter(qual, qual.begin()));
  return qual;
}

void ComplaintsAnswerManager::Clear()
{
  std::lock_guard<std::mutex> lock(mutex_);
  complaints_.clear();
  complaint_answers_received_.clear();
}

}  // namespace dkg
}  // namespace fetch
