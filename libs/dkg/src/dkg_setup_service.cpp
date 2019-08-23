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

#include "dkg/dkg_setup_service.hpp"
#include "telemetry/registry.hpp"

#include <mutex>

namespace fetch {
namespace dkg {

char const *ToString(DkgSetupService::State state)
{
  char const *text = "unknown";

  switch (state)
  {
  case DkgSetupService::State::INITIAL:
    text = "Initial";
    break;
  case DkgSetupService::State::WAITING_FOR_SHARE:
    text = "Waiting for shares and coefficients";
    break;
  case DkgSetupService::State::WAITING_FOR_COMPLAINTS:
    text = "Waiting for complaints";
    break;
  case DkgSetupService::State::WAITING_FOR_COMPLAINT_ANSWERS:
    text = "Wait for complaint answers";
    break;
  case DkgSetupService::State::WAITING_FOR_QUAL_SHARES:
    text = "Waiting for qual shares";
    break;
  case DkgSetupService::State::WAITING_FOR_QUAL_COMPLAINTS:
    text = "Waiting for qual complaints";
    break;
  case DkgSetupService::State::WAITING_FOR_RECONSTRUCTION_SHARES:
    text = "Waiting for reconstruction shares";
    break;
  case DkgSetupService::State::FINAL:
    text = "Dkg finished";
  }

  return text;
}

DkgSetupService::DkgSetupService(
    MuddleAddress address, std::function<void(DKGEnvelope const &)> broadcast_function,
    std::function<void(MuddleAddress const &, std::pair<std::string, std::string> const &)>
        rpc_function)
  : address_{std::move(address)}
  , broadcast_function_{std::move(broadcast_function)}
  , rpc_function_{std::move(rpc_function)}
  , dkg_state_gauge_{telemetry::Registry::Instance().CreateGauge<uint8_t>(
        "ledger_dkg_state_gauge", "State the DKG is in as integer in [0, 7]")}
  , state_machine_{std::make_shared<StateMachine>("BeaconSetupService", State::INITIAL, ToString)}
  , manager_{address_}
{
  state_machine_->RegisterHandler(State::INITIAL, this, &DkgSetupService::OnInitial);
  state_machine_->RegisterHandler(State::WAITING_FOR_SHARE, this,
                                  &DkgSetupService::OnWaitForShares);
  state_machine_->RegisterHandler(State::WAITING_FOR_COMPLAINTS, this,
                                  &DkgSetupService::OnWaitForComplaints);
  state_machine_->RegisterHandler(State::WAITING_FOR_COMPLAINT_ANSWERS, this,
                                  &DkgSetupService::OnWaitForComplaintAnswers);
  state_machine_->RegisterHandler(State::WAITING_FOR_QUAL_SHARES, this,
                                  &DkgSetupService::OnWaitForQualShares);
  state_machine_->RegisterHandler(State::WAITING_FOR_QUAL_COMPLAINTS, this,
                                  &DkgSetupService::OnWaitForQualComplaints);
  state_machine_->RegisterHandler(State::WAITING_FOR_RECONSTRUCTION_SHARES, this,
                                  &DkgSetupService::OnWaitForReconstructionShares);
  state_machine_->RegisterHandler(State::FINAL, this, &DkgSetupService::OnFinal);

  dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
}

DkgSetupService::State DkgSetupService::OnInitial()
{
  BroadcastShares();
  state_.store(State::WAITING_FOR_SHARE);
  dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
  return State::WAITING_FOR_SHARE;
}

DkgSetupService::State DkgSetupService::OnWaitForShares()
{
  std::unique_lock<std::mutex> lock{mutex_};
  if (C_ik_received_.load() == cabinet_.size() - 1 &&
      shares_received_.load() == cabinet_.size() - 1)
  {
    lock.unlock();
    BroadcastComplaints();
    state_ = State::WAITING_FOR_COMPLAINTS;
    dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
    return State::WAITING_FOR_COMPLAINTS;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAITING_FOR_SHARE;
}

DkgSetupService::State DkgSetupService::OnWaitForComplaints()
{
  std::unique_lock<std::mutex> lock{mutex_};
  if (complaints_manager_.IsFinished(manager_.polynomial_degree()))
  {
    // Complaints at this point consist only of parties which have received over threshold number of
    // complaints
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", manager_.cabinet_index(), " complaints size ",
                   complaints_manager_.Complaints().size());
    complaints_answer_manager_.Init(complaints_manager_.Complaints());
    lock.unlock();
    BroadcastComplaintsAnswer();
    state_ = State::WAITING_FOR_COMPLAINT_ANSWERS;
    dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
    return State::WAITING_FOR_COMPLAINT_ANSWERS;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAITING_FOR_COMPLAINTS;
}

DkgSetupService::State DkgSetupService::OnWaitForComplaintAnswers()
{
  std::unique_lock<std::mutex> lock{mutex_};
  if (complaints_answer_manager_.IsFinished())
  {
    lock.unlock();
    if (BuildQual())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", manager_.cabinet_index(), "build qual of size ",
                     qual_.size());
      manager_.ComputeSecretShare(qual_);
      BroadcastQualCoefficients();
      state_ = State::WAITING_FOR_QUAL_SHARES;
      dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
      complaints_manager_.Clear();
      return State::WAITING_FOR_QUAL_SHARES;
    }
    else
    {
      state_ = State::FINAL;
      dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
      complaints_manager_.Clear();
      // TODO(jmw): procedure failed for this node
      return State::FINAL;
    }
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAITING_FOR_COMPLAINT_ANSWERS;
}

DkgSetupService::State DkgSetupService::OnWaitForQualShares()
{
  std::set<MuddleAddress> diff;
  std::set_difference(qual_.begin(), qual_.end(), A_ik_received_.begin(), A_ik_received_.end(),
                      std::inserter(diff, diff.begin()));
  if (diff.empty())
  {
    BroadcastQualComplaints();
    state_ = State::WAITING_FOR_QUAL_COMPLAINTS;
    dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
    return State::WAITING_FOR_QUAL_COMPLAINTS;
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAITING_FOR_QUAL_SHARES;
}

DkgSetupService::State DkgSetupService::OnWaitForQualComplaints()
{
  if (qual_complaints_manager_.IsFinished(qual_, address_))
  {
    CheckQualComplaints();
    size_t size = qual_complaints_manager_.ComplaintsSize();

    if (size > manager_.polynomial_degree())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", manager_.cabinet_index(),
                     " DKG has failed: complaints size ", size, " greater than threshold.");
      state_ = State::FINAL;
      dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
      // TODO(jmw): Failure state
      return State::FINAL;
    }
    else if (qual_complaints_manager_.ComplaintsFind(address_))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", manager_.cabinet_index(), " is in qual complaints");
      manager_.ComputePublicKeys(qual_);
      state_ = State::FINAL;
      dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
      // TODO(jmw): Failure state
      return State::FINAL;
    }
    assert(qual_.find(address_) != qual_.end());
    BroadcastReconstructionShares();
    reconstruction_shares_received_.insert(address_);
    state_ = State::WAITING_FOR_RECONSTRUCTION_SHARES;
    dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
    return State::WAITING_FOR_RECONSTRUCTION_SHARES;
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAITING_FOR_QUAL_COMPLAINTS;
}

DkgSetupService::State DkgSetupService::OnWaitForReconstructionShares()
{
  std::unique_lock<std::mutex> lock{mutex_};
  std::set<MuddleAddress>      complaints_list = qual_complaints_manager_.Complaints();
  std::set<MuddleAddress>      remaining_honest;
  std::set<MuddleAddress>      diff;
  std::set_difference(qual_.begin(), qual_.end(), complaints_list.begin(), complaints_list.end(),
                      std::inserter(remaining_honest, remaining_honest.begin()));
  std::set_difference(remaining_honest.begin(), remaining_honest.end(),
                      reconstruction_shares_received_.begin(),
                      reconstruction_shares_received_.end(), std::inserter(diff, diff.begin()));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", manager_.cabinet_index(),
                  " reconstruction shares remaining ", diff.size());
  if (diff.empty())
  {
    lock.unlock();
    if (!manager_.RunReconstruction())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", manager_.cabinet_index(),
                     " DKG failed due to reconstruction failure");
      state_ = State::FINAL;
      dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
      // TODO(jmw): Failure state
      return State::FINAL;
    }
    else
    {
      manager_.ComputePublicKeys(qual_);
      state_ = State::FINAL;
      dkg_state_gauge_->set(static_cast<uint8_t>(state_.load()));
      qual_complaints_manager_.Clear();
      return State::FINAL;
    }
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAITING_FOR_RECONSTRUCTION_SHARES;
}

DkgSetupService::State DkgSetupService::OnFinal()
{
  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::FINAL;
}

/**
 * Sends DKG message via reliable broadcast channel in dkg_service
 *
 * @param env DKGEnvelope containing message to the broadcasted
 */
void DkgSetupService::SendBroadcast(DKGEnvelope const &env)
{
  broadcast_function_(env);
}

/**
 * Randomly initialises coefficients of two polynomials, computes the coefficients and secret
 * shares and sends to cabinet members
 */
void DkgSetupService::BroadcastShares()
{
  manager_.GenerateCoefficients();
  SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_SHARE),
                                                manager_.GetCoefficients(), "signature"}});
  for (auto &cab_i : cabinet_)
  {
    if (cab_i != address_)
    {
      std::pair<MessageShare, MessageShare> shares{manager_.GetOwnShares(cab_i)};
      rpc_function_(cab_i, shares);
    }
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", manager_.cabinet_index(), " broadcasts coefficients ");
}

/**
 * Broadcast the set nodes we are complaining against based on the secret shares and coefficients
 * sent to use. Also increments the number of complaints a given cabinet member has received with
 * our complaints
 */
void DkgSetupService::BroadcastComplaints()
{
  std::unordered_set<MuddleAddress> complaints_local = manager_.ComputeComplaints();
  for (auto const &cab : complaints_local)
  {
    complaints_manager_.Count(cab);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Node ", manager_.cabinet_index(), " broadcasts complaints size ",
                 complaints_local.size());
  SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local, "signature"}});
}

/**
 * For a complaint by cabinet member c_i against self we broadcast the secret share
 * we sent to c_i to all cabinet members. This serves as a round of defense against
 * complaints where a member reveals the secret share they sent to c_i to everyone to
 * prove that it is consistent with the coefficients they originally broadcasted
 */
void DkgSetupService::BroadcastComplaintsAnswer()
{
  std::unordered_map<MuddleAddress, std::pair<MessageShare, MessageShare>> complaints_answer;
  for (auto const &reporter : complaints_manager_.ComplaintsFrom())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", manager_.cabinet_index(), " received complaints from ",
                   manager_.cabinet_index(reporter));
    complaints_answer.insert({reporter, manager_.GetOwnShares(reporter)});
  }
  SendBroadcast(
      DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_COMPLAINT_ANSWERS),
                                complaints_answer, "signature"}});
}

/**
 * Broadcast coefficients after computing own secret share
 */
void DkgSetupService::BroadcastQualCoefficients()
{
  SendBroadcast(
      DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAITING_FOR_QUAL_SHARES),
                                      manager_.GetQualCoefficients(), "signature"}});
  complaints_answer_manager_.Clear();
  {
    std::unique_lock<std::mutex> lock{mutex_};
    A_ik_received_.insert(address_);
  }
}

/**
 * After constructing the qualified set (qual) and receiving new qual coefficients members
 * broadcast the secret shares s_ij, sprime_ij of all members in qual who sent qual coefficients
 * which failed verification
 */
void DkgSetupService::BroadcastQualComplaints()
{
  SendBroadcast(DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_QUAL_COMPLAINTS),
                                          manager_.ComputeQualComplaints(qual_), "signature"}});
}

/**
 * For all members that other nodes have complained against in qual we also broadcast
 * the secret shares we received from them to all cabinet members and collect the shares broadcasted
 * by others
 */

void DkgSetupService::BroadcastReconstructionShares()
{
  SharesExposedMap complaint_shares;
  for (auto const &in : qual_complaints_manager_.Complaints())
  {
    assert(qual_.find(in) != qual_.end());
    manager_.AddReconstructionShare(in);
    complaint_shares.insert({in, manager_.GetReceivedShares(in)});
  }
  SendBroadcast(
      DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAITING_FOR_RECONSTRUCTION_SHARES),
                                complaint_shares, "signature"}});
}

/**
 * Handler for DKGMessage that has passed through the reliable broadcast
 *
 * @param from Muddle address of sender
 * @param msg_ptr Pointer of DKGMessage
 */
void DkgSetupService::OnDkgMessage(MuddleAddress const &from, std::shared_ptr<DKGMessage> msg_ptr)
{
  if (!BasicMsgCheck(from, msg_ptr))
  {
    return;
  }
  switch (msg_ptr->type())
  {
  case DKGMessage::MessageType::COEFFICIENT:
  {
    auto coefficients_ptr = std::dynamic_pointer_cast<CoefficientsMessage>(msg_ptr);
    if (coefficients_ptr != nullptr)
    {
      OnNewCoefficients(*coefficients_ptr, from);
    }
    break;
  }
  case DKGMessage::MessageType::SHARE:
  {
    auto share_ptr = std::dynamic_pointer_cast<SharesMessage>(msg_ptr);
    if (share_ptr != nullptr)
    {
      OnExposedShares(*share_ptr, from);
    }
    break;
  }
  case DKGMessage::MessageType::COMPLAINT:
  {
    auto complaint_ptr = std::dynamic_pointer_cast<ComplaintsMessage>(msg_ptr);
    if (complaint_ptr != nullptr)
    {
      OnComplaints(*complaint_ptr, from);
    }
    break;
  }
  default:
    FETCH_LOG_ERROR(LOGGING_NAME, "Node: ", manager_.cabinet_index(),
                    " can not process payload from node ", manager_.cabinet_index(from));
  }
}

/**
 * Handler for all broadcasted messages containing secret shares
 *
 * @param shares Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void DkgSetupService::OnExposedShares(SharesMessage const &shares, MuddleAddress const &from_id)
{
  uint64_t phase1     = shares.phase();
  uint32_t from_index = manager_.cabinet_index(from_id);
  if (phase1 == static_cast<uint64_t>(State::WAITING_FOR_COMPLAINT_ANSWERS))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", manager_.cabinet_index(),
                   " received complaint answer from ", from_index);
    OnComplaintsAnswer(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAITING_FOR_QUAL_COMPLAINTS))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", manager_.cabinet_index(),
                   " received QUAL complaint from ", from_index);
    OnQualComplaints(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAITING_FOR_RECONSTRUCTION_SHARES))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", manager_.cabinet_index(),
                   " received reconstruction share from ", from_index);
    OnReconstructionShares(shares, from_id);
  }
}

/**
 * Handler for RPC submit shares used for members to send individual pairs of
 * secret shares to other cabinet members
 *
 * @param from Muddle address of sender
 * @param shares Pair of secret shares
 */
void DkgSetupService::OnNewShares(MuddleAddress                                from,
                                  std::pair<MessageShare, MessageShare> const &shares)
{
  if (manager_.AddShares(from, shares))
  {
    ++shares_received_;
  }
}

/**
 * Handler for broadcasted coefficients
 *
 * @param msg_ptr Pointer of CoefficientsMessage
 * @param from_id Muddle address of sender
 */
void DkgSetupService::OnNewCoefficients(CoefficientsMessage const &msg,
                                        MuddleAddress const &      from_id)
{
  if (msg.phase() == static_cast<uint64_t>(State::WAITING_FOR_SHARE))
  {
    if (manager_.AddCoefficients(from_id, msg.coefficients()))
    {
      ++C_ik_received_;
    }
  }
  else if (msg.phase() == static_cast<uint64_t>(State::WAITING_FOR_QUAL_SHARES))
  {
    if (manager_.AddQualCoefficients(from_id, msg.coefficients()))
    {
      std::unique_lock<std::mutex> lock{mutex_};
      A_ik_received_.insert(from_id);
    }
  }
}

/**
 * Handler for complaints message
 *
 * @param msg_ptr Pointer of ComplaintsMessage
 * @param from_id Muddle address of sender
 */
void DkgSetupService::OnComplaints(ComplaintsMessage const &msg, MuddleAddress const &from_id)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", manager_.cabinet_index(), " received complaints from node ",
                 manager_.cabinet_index(from_id));
  complaints_manager_.Add(msg, from_id, address_);
}

/**
 * Handler for complaints answer message containing the pairs of secret shares the sender sent to
 * members that complained against the sender
 *
 * @param msg_ptr Pointer of SharesMessage containing the sender's shares
 * @param from_id Muddle address of sender
 */
void DkgSetupService::OnComplaintsAnswer(SharesMessage const &answer, MuddleAddress const &from_id)
{
  if (complaints_answer_manager_.Count(from_id))
  {
    CheckComplaintAnswer(answer, from_id);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", manager_.cabinet_index(),
                   " received multiple complaint answer from node ",
                   manager_.cabinet_index(from_id));
  }
}

/**
 * Handler for qual complaints message which contains the secret shares sender received from
 * members in qual complaints
 *
 * @param shares_ptr Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void DkgSetupService::OnQualComplaints(SharesMessage const &shares_msg,
                                       MuddleAddress const &from_id)
{
  qual_complaints_manager_.Received(from_id, shares_msg.shares());
}

/**
 * Handler for messages containing secret shares of qual members that other qual members have
 * complained against
 *
 * @param shares_ptr Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void DkgSetupService::OnReconstructionShares(SharesMessage const &shares_msg,
                                             MuddleAddress const &from_id)
{
  // Return if the sender is in complaints, or not in QUAL
  // TODO(JMW): Could be problematic if qual has not been built yet
  if (qual_complaints_manager_.ComplaintsFind(from_id) or qual_.find(from_id) == qual_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", manager_.cabinet_index(),
                   " received message from invalid sender. Discarding.");
    return;
  }
  for (auto const &share : shares_msg.shares())
  {
    if (manager_.CheckDuplicateReconstructionShare(from_id, share.first))
    {
      return;
    }
    manager_.VerifyReconstructionShare(from_id, share);
  }
  reconstruction_shares_received_.insert(from_id);
}

/**
 * For all complaint answers received in defense of a complaint we check the exposed secret share
 * is consistent with the broadcasted coefficients
 *
 * @param answer Pointer of message containing some secret shares of sender
 * @param from_id Muddle address of sender
 * @param from_index Index of sender in cabinet_
 */
void DkgSetupService::CheckComplaintAnswer(SharesMessage const &answer,
                                           MuddleAddress const &from_id)
{
  // If not enough answers are sent for number of complaints against a node then add a complaint a
  // against it
  auto diff = complaints_manager_.ComplaintsCount(from_id) - answer.shares().size();
  if (diff > 0 && diff <= cabinet_.size())
  {
    complaints_answer_manager_.Add(from_id);
  }
  for (auto const &share : answer.shares())
  {
    if (!manager_.VerifyComplaintAnswer(from_id, share))
    {
      complaints_answer_manager_.Add(from_id);
    }
  }
}

/**
 * Builds the set of qualified members of the cabinet.  Altogether, complaints consists of
  // 1. Nodes which received over t complaints
  // 2. Complaint answers which were false

 * @return True if self is in qual and qual is at least of size polynomial_degree_ + 1, false
 otherwise
 */
bool DkgSetupService::BuildQual()
{
  qual_ = complaints_answer_manager_.BuildQual(cabinet_);
  if (qual_.find(address_) == qual_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node: ", manager_.cabinet_index(),
                   " build QUAL failed as not in QUAL");
    return false;
  }
  else if (qual_.size() <= manager_.polynomial_degree())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node: ", manager_.cabinet_index(), " build QUAL failed as size ",
                   qual_.size(), " less than threshold ", manager_.polynomial_degree());
    return false;
  }
  return true;
}

/**
 * Checks the complaints set by qual members
 */
void DkgSetupService::CheckQualComplaints()
{
  for (const auto &complaint : qual_complaints_manager_.ComplaintsReceived())
  {
    MuddleAddress sender = complaint.first;
    // Return if the sender not in QUAL
    if (qual_.find(sender) == qual_.end())
    {
      return;
    }
    for (auto const &share : complaint.second)
    {
      // Check person who's shares are being exposed is not in QUAL then don't bother with checks
      if (qual_.find(share.first) != qual_.end())
      {
        qual_complaints_manager_.Complaints(manager_.VerifyQualComplaint(sender, share));
      }
    }
  }
}

/**
 * Helper function to check basic details of the message to determine if it should be processed
 *
 * @param from Muddle address of sender
 * @param msg_ptr Shared pointer of message
 * @return Bool of whether the message passes the test or not
 */
bool DkgSetupService::BasicMsgCheck(MuddleAddress const &              from,
                                    std::shared_ptr<DKGMessage> const &msg_ptr)
{
  if (msg_ptr == nullptr)
  {
    return false;
  }
  else if (cabinet_.find(from) == cabinet_.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", manager_.cabinet_index(),
                   " received message from unknown sender");
    return false;
  }
  return true;
}

/**
 * Resets cabinet for next round of DKG
 */
void DkgSetupService::ResetCabinet(CabinetMembers const &cabinet, uint32_t threshold)
{
  assert(cabinet.find(address_) != cabinet.end());  // We should be in the cabinet

  std::lock_guard<std::mutex> lock_{mutex_};
  cabinet_ = cabinet;
  state_   = State::INITIAL;
  manager_.Reset(cabinet, threshold);
  auto cabinet_size{static_cast<uint32_t>(cabinet_.size())};
  qual_.clear();

  complaints_manager_.ResetCabinet(cabinet_size);
  complaints_answer_manager_.ResetCabinet(cabinet_size);
  qual_complaints_manager_.Reset();

  shares_received_ = 0;
  C_ik_received_   = 0;

  A_ik_received_.clear();
  reconstruction_shares_received_.clear();
}

/**
 * @return Whether DKG has finished
 */
bool DkgSetupService::finished() const
{
  return (state_.load() == State::FINAL);
}

void DkgSetupService::SetDkgOutput(bn::G2 &public_key, bn::Fr &secret_share,
                                   std::vector<bn::G2> &    public_key_shares,
                                   std::set<MuddleAddress> &qual)
{
  manager_.SetDkgOutput(public_key, secret_share, public_key_shares);
  qual = qual_;
}

std::weak_ptr<core::Runnable> DkgSetupService::GetWeakRunnable()
{
  return state_machine_;
}

}  // namespace dkg
}  // namespace fetch
