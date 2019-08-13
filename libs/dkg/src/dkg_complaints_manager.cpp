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
  std::lock_guard<std::mutex> lock{mutex_};
  cabinet_size_        = cabinet_size;
  finished_            = false;
  complaints_received_ = std::vector<bool>(cabinet_size_, false);
  complaints_counter_.clear();
  complaints_from_.clear();
  complaints_.clear();
  complaints_received_.clear();
  complaints_received_counter_ = 0;
}

void ComplaintsManager::Count(MuddleAddress const &address)
{
  std::lock_guard<std::mutex> lock(mutex_);
  ++complaints_counter_[address];
}

void ComplaintsManager::Add(ComplaintsMessage const &msg, MuddleAddress const &from_id,
                            uint32_t from_index, MuddleAddress const &address)
{
  std::lock_guard<std::mutex> lock{mutex_};
  // Check if we have received a complaints message from this node before and if not log that we
  // received a complaint message
  if (!complaints_received_[from_index])
  {
    complaints_received_[from_index] = true;
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Duplicate complaints received from node ", from_index);
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
  ++complaints_received_counter_;
}

bool ComplaintsManager::IsFinished(std::set<MuddleAddress> const &miners, uint32_t node_index,
                                   uint32_t threshold)
{
  std::lock_guard<std::mutex> lock{mutex_};
  if (complaints_received_counter_ == cabinet_size_ - 1)
  {
    // Add miners which did not send a complaint to complaints (redundant for now but will be
    // necessary when we do not wait for a message from everyone)
    auto miner_it = miners.begin();
    for (uint32_t ii = 0; ii < complaints_received_.size(); ++ii)
    {
      if (!complaints_received_[ii] && ii != node_index)
      {
        complaints_.insert(*miner_it);
      }
      ++miner_it;
    }
    // All miners who have received over t complaints are also disqualified
    for (auto const &node_complaints : complaints_counter_)
    {
      if (node_complaints.second > threshold)
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
  assert(finished_);
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
  assert(finished_);
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
                                       MuddleAddress const &          node_id)
{
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto &member : qual)
  {
    if (member != node_id && complaints_received_.find(member) == complaints_received_.end())
    {
      return false;
    }
  }
  finished_ = true;
  return true;
}

void QualComplaintsManager::Clear()
{
  std::lock_guard<std::mutex> lock{mutex_};
  assert(finished_);
  complaints_.clear();
  complaints_received_.clear();
}

void QualComplaintsManager::Reset()
{
  std::lock_guard<std::mutex> lock{mutex_};
  finished_ = false;
  complaints_.clear();
  complaints_received_.clear();
}

void ComplaintsAnswerManager::Init(std::set<MuddleAddress> const &complaints)
{
  std::lock_guard<std::mutex> lock{mutex_};
  std::copy(complaints.begin(), complaints.end(), std::inserter(complaints_, complaints_.begin()));
}

void ComplaintsAnswerManager::ResetCabinet(uint32_t cabinet_size)
{
  std::lock_guard<std::mutex> lock{mutex_};
  cabinet_size_               = cabinet_size;
  finished_                   = false;
  complaint_answers_received_ = std::vector<bool>(cabinet_size_, false);
  complaints_.clear();
  complaint_answers_received_counter_ = 0;
}

void ComplaintsAnswerManager::Add(MuddleAddress const &member)
{
  std::lock_guard<std::mutex> lock{mutex_};
  complaints_.insert(member);
}

bool ComplaintsAnswerManager::Count(uint32_t from_index)
{
  std::lock_guard<std::mutex> lock{mutex_};
  if (!complaint_answers_received_[from_index])
  {
    complaint_answers_received_[from_index] = true;
    ++complaint_answers_received_counter_;
    return true;
  }
  else
  {
    return false;
  }
}

bool ComplaintsAnswerManager::IsFinished(std::set<MuddleAddress> const &cabinet, uint32_t index)
{
  std::lock_guard<std::mutex> lock{mutex_};
  if (complaint_answers_received_counter_ == cabinet_size_ - 1)
  {
    // Add miners which did not send a complaint to complaints (redundant for now but will be
    // necessary when we do not wait for a message from everyone)
    auto miner_it = cabinet.begin();
    for (uint32_t ii = 0; ii < complaint_answers_received_.size(); ++ii)
    {
      if (!complaint_answers_received_[ii] && ii != index)
      {
        complaints_.insert(*miner_it);
      }
      ++miner_it;
    }
    finished_ = true;
    return true;
  }
  return false;
}

std::set<ComplaintsAnswerManager::MuddleAddress> ComplaintsAnswerManager::BuildQual(
    std::set<MuddleAddress> const &cabinet)
{
  std::lock_guard<std::mutex> lock{mutex_};
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
