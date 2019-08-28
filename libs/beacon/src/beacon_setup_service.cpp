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

#include "beacon/beacon_setup_service.hpp"
#include "telemetry/registry.hpp"

#include <mutex>

namespace fetch {
namespace beacon {

char const *ToString(BeaconSetupService::State state)
{
  char const *text = "unknown";

  switch (state)
  {
  case BeaconSetupService::State::IDLE:
    text = "Idle";
    break;
  case BeaconSetupService::State::WAIT_FOR_DIRECT_CONNECTIONS:
    text = "Waiting for direct connections";
    break;
  case BeaconSetupService::State::WAIT_FOR_READY_CONNECTIONS:
    text = "Waiting for ready connections";
    break;
  case BeaconSetupService::State::WAIT_FOR_SHARE:
    text = "Waiting for shares and coefficients";
    break;
  case BeaconSetupService::State::WAIT_FOR_COMPLAINTS:
    text = "Waiting for complaints";
    break;
  case BeaconSetupService::State::WAIT_FOR_COMPLAINT_ANSWERS:
    text = "Wait for complaint answers";
    break;
  case BeaconSetupService::State::WAIT_FOR_QUAL_SHARES:
    text = "Waiting for qual shares";
    break;
  case BeaconSetupService::State::WAIT_FOR_QUAL_COMPLAINTS:
    text = "Waiting for qual complaints";
    break;
  case BeaconSetupService::State::WAIT_FOR_RECONSTRUCTION_SHARES:
    text = "Waiting for reconstruction shares";
    break;
  case BeaconSetupService::State::BEACON_READY:
    text = "Beacon ready";
  }

  return text;
}

BeaconSetupService::BeaconSetupService(Endpoint &endpoint, Identity identity)
  : identity_{std::move(identity)}
  , endpoint_{endpoint}
  , shares_subscription(endpoint_.Subscribe(SERVICE_DKG, CHANNEL_SECRET_KEY))
  , pre_dkg_rbc_{endpoint_, identity_.identifier(),
                 [this](MuddleAddress const &from, ConstByteArray const &payload) -> void {
                   std::set<MuddleAddress>               connections;
                   fetch::serializers::MsgPackSerializer serializer{payload};
                   serializer >> connections;

                   if (ready_connections_.find(from) == ready_connections_.end())
                   {
                     ready_connections_.insert({from, connections});
                   }
                 },
                 CHANNEL_CONNECTIONS_SETUP, false}
  , rbc_{endpoint_, identity_.identifier(),
         [this](MuddleAddress const &from, ConstByteArray const &payload) -> void {
           DKGEnvelope   env;
           DKGSerializer serializer{payload};
           serializer >> env;
           OnDkgMessage(from, env.Message());
         },
         CHANNEL_RBC_BROADCAST}
  , state_machine_{std::make_shared<StateMachine>("BeaconSetupService", State::IDLE, ToString)}
  , dkg_state_gauge_{telemetry::Registry::Instance().CreateGauge<uint8_t>(
        "ledger_dkg_state_gauge", "State the DKG is in as integer in [0, 9]")}
{
  state_machine_->RegisterHandler(State::IDLE, this, &BeaconSetupService::OnIdle);
  state_machine_->RegisterHandler(State::WAIT_FOR_DIRECT_CONNECTIONS, this,
                                  &BeaconSetupService::OnWaitForDirectConnections);
  state_machine_->RegisterHandler(State::WAIT_FOR_READY_CONNECTIONS, this,
                                  &BeaconSetupService::OnWaitForReadyConnections);
  state_machine_->RegisterHandler(State::WAIT_FOR_SHARE, this,
                                  &BeaconSetupService::OnWaitForShares);
  state_machine_->RegisterHandler(State::WAIT_FOR_COMPLAINTS, this,
                                  &BeaconSetupService::OnWaitForComplaints);
  state_machine_->RegisterHandler(State::WAIT_FOR_COMPLAINT_ANSWERS, this,
                                  &BeaconSetupService::OnWaitForComplaintAnswers);
  state_machine_->RegisterHandler(State::WAIT_FOR_QUAL_SHARES, this,
                                  &BeaconSetupService::OnWaitForQualShares);
  state_machine_->RegisterHandler(State::WAIT_FOR_QUAL_COMPLAINTS, this,
                                  &BeaconSetupService::OnWaitForQualComplaints);
  state_machine_->RegisterHandler(State::WAIT_FOR_RECONSTRUCTION_SHARES, this,
                                  &BeaconSetupService::OnWaitForReconstructionShares);
  state_machine_->RegisterHandler(State::BEACON_READY, this, &BeaconSetupService::OnBeaconReady);

  // Set subscription for receiving shares
  shares_subscription->SetMessageHandler([this](ConstByteArray const &from, uint16_t, uint16_t,
                                                uint16_t, muddle::Packet::Payload const &payload,
                                                ConstByteArray) {
    fetch::serializers::MsgPackSerializer serialiser(payload);

    std::pair<std::string, std::string> shares;
    serialiser >> shares;

    // Dispatch the event
    OnNewShares(from, shares);
  });
}

BeaconSetupService::State BeaconSetupService::OnIdle()
{
  std::lock_guard<std::mutex> lock(mutex_);
  dkg_state_gauge_->set(static_cast<uint8_t>(State::IDLE));

  if (!aeon_exe_queue_.empty())
  {
    beacon_ = aeon_exe_queue_.front();
    assert(beacon_ != nullptr);

    aeon_exe_queue_.pop_front();

    // Observe only does not require any setup
    if (beacon_->observe_only)
    {
      return State::BEACON_READY;
    }
    else
    {
      // Initiating setup
      std::set<MuddleAddress> cabinet;
      for (auto &m : beacon_->aeon.members)
      {
        cabinet.insert(m.identifier());
      }

      pre_dkg_rbc_.ResetCabinet(cabinet);
      rbc_.ResetCabinet(cabinet);
      auto cabinet_size = static_cast<uint32_t>(cabinet.size());
      complaints_manager_.ResetCabinet(cabinet_size);
      complaints_answer_manager_.ResetCabinet(cabinet_size);
      qual_complaints_manager_.Reset();

      return State::WAIT_FOR_DIRECT_CONNECTIONS;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(100));
  return State::IDLE;
}

BeaconSetupService::State BeaconSetupService::OnWaitForDirectConnections()
{
  std::lock_guard<std::mutex> lock(mutex_);
  dkg_state_gauge_->set(static_cast<uint8_t>(State::WAIT_FOR_DIRECT_CONNECTIONS));

  auto                              v_peers = endpoint_.GetDirectlyConnectedPeers();
  std::unordered_set<MuddleAddress> peers(v_peers.begin(), v_peers.end());

  std::set<MuddleAddress> connected;

  for (auto &m : beacon_->aeon.members)
  {
    // Skipping own address
    if (m == identity_)
    {
      connected.insert(m.identifier());
      continue;
    }

    // Checking if other peers are there
    if (peers.find(m.identifier()) == peers.end())
    {
      // TODO(tfr): Request muddle to connect.
    }
    else
    {
      connected.insert(m.identifier());
    }
  }

  if (connected.size() == beacon_->aeon.members.size())
  {
    connections_ = connected;
    fetch::serializers::MsgPackSerializer serializer;
    serializer << connected;
    pre_dkg_rbc_.Broadcast(serializer.data());
    return State::WAIT_FOR_READY_CONNECTIONS;
  }

  state_machine_->Delay(std::chrono::milliseconds(200));
  FETCH_LOG_INFO(LOGGING_NAME, "Waiting for all peers to join before starting setup. Connected: ",
                 connected.size(), " expect: ", beacon_->aeon.members.size() - 1);

  return State::WAIT_FOR_DIRECT_CONNECTIONS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForReadyConnections()
{
  std::lock_guard<std::mutex> lock(mutex_);
  dkg_state_gauge_->set(static_cast<uint8_t>(State::WAIT_FOR_READY_CONNECTIONS));

  // Try broadcasting connections again in case they were discarded by RBCs who's cabinet was reset
  // late
  fetch::serializers::MsgPackSerializer serializer;
  serializer << connections_;
  pre_dkg_rbc_.Broadcast(serializer.data());

  if (ready_connections_.size() >= beacon_->aeon.members.size() - 1)
  {
    for (auto &m : beacon_->aeon.members)
    {
      if (m == identity_)
      {
        continue;
      }
      if (ready_connections_.find(m.identifier()) != ready_connections_.end())
      {
        assert(ready_connections_.at(m.identifier()) == connections_);
        // TODO(jmw): Strategy if connections for members are different
      }
      else
      {
        state_machine_->Delay(std::chrono::milliseconds(100));
        FETCH_LOG_INFO(LOGGING_NAME,
                       "Waiting for all peers to be ready before starting DKG. Ready: ",
                       ready_connections_.size(), " expect: ", beacon_->aeon.members.size() - 1);
        return State::WAIT_FOR_READY_CONNECTIONS;
      }
    }
    FETCH_LOG_INFO(LOGGING_NAME, "All peers connected. Proceeding.");

    BroadcastShares();
    return State::WAIT_FOR_SHARE;
  }

  state_machine_->Delay(std::chrono::milliseconds(100));
  FETCH_LOG_INFO(LOGGING_NAME, "Waiting for all peers to be ready before starting DKG. Ready: ",
                 ready_connections_.size(), " expect: ", beacon_->aeon.members.size() - 1);
  return State::WAIT_FOR_READY_CONNECTIONS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForShares()
{
  std::unique_lock<std::mutex> lock{mutex_};
  dkg_state_gauge_->set(static_cast<uint8_t>(State::WAIT_FOR_SHARE));

  if ((coefficients_received_.size() == beacon_->aeon.members.size() - 1) &&
      (shares_received_.size() == beacon_->aeon.members.size() - 1))
  {
    lock.unlock();
    BroadcastComplaints();

    // Clean up
    coefficients_received_.clear();
    shares_received_.clear();

    return State::WAIT_FOR_COMPLAINTS;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_SHARE;
}

BeaconSetupService::State BeaconSetupService::OnWaitForComplaints()
{
  std::unique_lock<std::mutex> lock{mutex_};
  dkg_state_gauge_->set(static_cast<uint8_t>(State::WAIT_FOR_COMPLAINTS));

  if (complaints_manager_.IsFinished(beacon_->manager.polynomial_degree()))
  {
    // Complaints at this point consist only of parties which have received over threshold number of
    // complaints
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), " complaints size ",
                   complaints_manager_.Complaints().size());
    complaints_answer_manager_.Init(complaints_manager_.Complaints());
    lock.unlock();
    BroadcastComplaintsAnswer();

    return State::WAIT_FOR_COMPLAINT_ANSWERS;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_COMPLAINTS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForComplaintAnswers()
{
  std::unique_lock<std::mutex> lock{mutex_};
  dkg_state_gauge_->set(static_cast<uint8_t>(State::WAIT_FOR_COMPLAINT_ANSWERS));

  if (complaints_answer_manager_.IsFinished())
  {
    lock.unlock();
    if (BuildQual())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), "build qual size ",
                     beacon_->manager.qual().size());
      beacon_->manager.ComputeSecretShare();
      BroadcastQualCoefficients();

      complaints_manager_.Clear();
      return State::WAIT_FOR_QUAL_SHARES;
    }
    else
    {
      complaints_manager_.Clear();
      // TODO(jmw): procedure failed for this node
      return State::BEACON_READY;
    }
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_COMPLAINT_ANSWERS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForQualShares()
{
  dkg_state_gauge_->set(static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES));

  std::set<MuddleAddress> diff;
  std::set<MuddleAddress> qual{beacon_->manager.qual()};
  std::set_difference(qual.begin(), qual.end(), qual_coefficients_received_.begin(),
                      qual_coefficients_received_.end(), std::inserter(diff, diff.begin()));
  if (diff.empty())
  {
    BroadcastQualComplaints();

    // Clean up
    qual_coefficients_received_.clear();

    return State::WAIT_FOR_QUAL_COMPLAINTS;
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_QUAL_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnWaitForQualComplaints()
{
  dkg_state_gauge_->set(static_cast<uint8_t>(State::WAIT_FOR_QUAL_COMPLAINTS));

  if (qual_complaints_manager_.IsFinished(beacon_->manager.qual(), identity_.identifier()))
  {
    CheckQualComplaints();
    size_t size = qual_complaints_manager_.ComplaintsSize();

    if (size > beacon_->manager.polynomial_degree())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                     " DKG has failed: complaints size ", size, " greater than threshold.");
      return State::BEACON_READY;
    }
    else if (qual_complaints_manager_.ComplaintsFind(identity_.identifier()))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                     " is in qual complaints");
      beacon_->manager.ComputePublicKeys();

      return State::BEACON_READY;
    }
    BroadcastReconstructionShares();

    return State::WAIT_FOR_RECONSTRUCTION_SHARES;
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_QUAL_COMPLAINTS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForReconstructionShares()
{
  std::unique_lock<std::mutex> lock{mutex_};
  dkg_state_gauge_->set(static_cast<uint8_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES));

  std::set<MuddleAddress> complaints_list = qual_complaints_manager_.Complaints();
  std::set<MuddleAddress> remaining_honest;
  std::set<MuddleAddress> qual{beacon_->manager.qual()};
  std::set_difference(qual.begin(), qual.end(), complaints_list.begin(), complaints_list.end(),
                      std::inserter(remaining_honest, remaining_honest.begin()));
  bool received_all{true};
  for (auto const &member : remaining_honest)
  {
    if (member != identity_.identifier() &&
        reconstruction_shares_received_.find(member) == reconstruction_shares_received_.end())
    {
      received_all = false;
      break;
    }
  }
  if (received_all)
  {
    // Process reconstruction shares. Return if the sender is in complaints, or not in QUAL
    for (auto const &share : reconstruction_shares_received_)
    {
      MuddleAddress from = share.first;
      if (qual_complaints_manager_.ComplaintsFind(from) ||
          beacon_->manager.qual().find(from) == beacon_->manager.qual().end())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                       " received message from invalid sender. Discarding.");
        continue;
      }
      for (auto const &elem : share.second)
      {
        beacon_->manager.VerifyReconstructionShare(from, elem);
      }
    }
    lock.unlock();
    if (!beacon_->manager.RunReconstruction())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                     " DKG failed due to reconstruction failure");
    }
    else
    {
      beacon_->manager.ComputePublicKeys();
    }
    return State::BEACON_READY;
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_RECONSTRUCTION_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnBeaconReady()
{
  std::lock_guard<std::mutex> lock(mutex_);
  dkg_state_gauge_->set(static_cast<uint8_t>(State::BEACON_READY));

  if (callback_function_)
  {
    callback_function_(beacon_);
  }

  beacon_.reset();
  pre_dkg_rbc_.ResetCabinet({});
  rbc_.ResetCabinet({});
  connections_.clear();
  ready_connections_.clear();
  complaints_manager_.ResetCabinet(0);
  complaints_answer_manager_.ResetCabinet(0);
  qual_complaints_manager_.Reset();
  shares_received_.clear();
  coefficients_received_.clear();
  qual_coefficients_received_.clear();
  reconstruction_shares_received_.clear();

  return State::IDLE;
}

/**
 * Sends DKG message via reliable broadcast channel in dkg_service
 *
 * @param env DKGEnvelope containing message to the broadcasted
 */
void BeaconSetupService::SendBroadcast(DKGEnvelope const &env)
{
  DKGSerializer serialiser;
  serialiser << env;
  rbc_.Broadcast(serialiser.data());
}

/**
 * Randomly initialises coefficients of two polynomials, computes the coefficients and secret
 * shares and sends to cabinet members
 */
void BeaconSetupService::BroadcastShares()
{
  beacon_->manager.GenerateCoefficients();
  SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARE),
                                                beacon_->manager.GetCoefficients(), "signature"}});
  for (auto &cab_i : beacon_->aeon.members)
  {
    if (cab_i == identity_)
    {
      continue;
    }
    std::pair<MessageShare, MessageShare> shares{beacon_->manager.GetOwnShares(cab_i.identifier())};

    fetch::serializers::SizeCounter counter;
    counter << shares;

    fetch::serializers::MsgPackSerializer serializer;
    serializer.Reserve(counter.size());
    serializer << shares;
    endpoint_.Send(cab_i.identifier(), SERVICE_DKG, CHANNEL_SECRET_KEY, serializer.data());
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                 " broadcasts coefficients ");
}

/**
 * Broadcast the set nodes we are complaining against based on the secret shares and coefficients
 * sent to use. Also increments the number of complaints a given cabinet member has received with
 * our complaints
 */
void BeaconSetupService::BroadcastComplaints()
{
  std::unordered_set<MuddleAddress> complaints_local = beacon_->manager.ComputeComplaints();
  for (auto const &cab : complaints_local)
  {
    complaints_manager_.Count(cab);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                 " broadcasts complaints size ", complaints_local.size());
  SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local, "signature"}});
}

/**
 * For a complaint by cabinet member c_i against self we broadcast the secret share
 * we sent to c_i to all cabinet members. This serves as a round of defense against
 * complaints where a member reveals the secret share they sent to c_i to everyone to
 * prove that it is consistent with the coefficients they originally broadcasted
 */
void BeaconSetupService::BroadcastComplaintsAnswer()
{
  std::unordered_map<MuddleAddress, std::pair<MessageShare, MessageShare>> complaints_answer;
  for (auto const &reporter : complaints_manager_.ComplaintsFrom())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " received complaints from ", beacon_->manager.cabinet_index(reporter));
    complaints_answer.insert({reporter, beacon_->manager.GetOwnShares(reporter)});
  }
  SendBroadcast(DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS),
                                          complaints_answer, "signature"}});
}

/**
 * Broadcast coefficients after computing own secret share
 */
void BeaconSetupService::BroadcastQualCoefficients()
{
  SendBroadcast(
      DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES),
                                      beacon_->manager.GetQualCoefficients(), "signature"}});
  complaints_answer_manager_.Clear();
  {
    std::unique_lock<std::mutex> lock{mutex_};
    qual_coefficients_received_.insert(identity_.identifier());
  }
}

/**
 * After constructing the qualified set (qual) and receiving new qual coefficients members
 * broadcast the secret shares s_ij, sprime_ij of all members in qual who sent qual coefficients
 * which failed verification
 */
void BeaconSetupService::BroadcastQualComplaints()
{
  SendBroadcast(DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
                                          beacon_->manager.ComputeQualComplaints(), "signature"}});
}

/**
 * For all members that other nodes have complained against in qual we also broadcast
 * the secret shares we received from them to all cabinet members and collect the shares broadcasted
 * by others
 */

void BeaconSetupService::BroadcastReconstructionShares()
{
  SharesExposedMap complaint_shares;
  for (auto const &in : qual_complaints_manager_.Complaints())
  {
    beacon_->manager.AddReconstructionShare(in);
    complaint_shares.insert({in, beacon_->manager.GetReceivedShares(in)});
  }
  SendBroadcast(
      DKGEnvelope{SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES),
                                complaint_shares, "signature"}});
}

/**
 * Handler for DKGMessage that has passed through the reliable broadcast
 *
 * @param from Muddle address of sender
 * @param msg_ptr Pointer of DKGMessage
 */
void BeaconSetupService::OnDkgMessage(MuddleAddress const &       from,
                                      std::shared_ptr<DKGMessage> msg_ptr)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (state_machine_->state() == State::IDLE || !BasicMsgCheck(from, msg_ptr))
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
    FETCH_LOG_ERROR(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                    " can not process payload from node ", beacon_->manager.cabinet_index(from));
  }
}

/**
 * Handler for all broadcasted messages containing secret shares
 *
 * @param shares Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void BeaconSetupService::OnExposedShares(SharesMessage const &shares, MuddleAddress const &from_id)
{
  uint64_t phase1     = shares.phase();
  uint32_t from_index = beacon_->manager.cabinet_index(from_id);
  if (phase1 == static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                   " received complaint answer from ", from_index);
    OnComplaintsAnswer(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                   " received QUAL complaint from ", from_index);
    OnQualComplaints(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
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
void BeaconSetupService::OnNewShares(MuddleAddress                                from,
                                     std::pair<MessageShare, MessageShare> const &shares)
{
  // Check if sender is in cabinet
  bool in_cabinet{false};
  for (auto &m : beacon_->aeon.members)
  {
    if (m.identifier() == from)
    {
      in_cabinet = true;
    }
  }
  if (state_machine_->state() == State::IDLE || !in_cabinet)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " received shares while idle or from unknown sender");
    return;
  }

  if (shares_received_.find(from) == shares_received_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " received shares from node  ", beacon_->manager.cabinet_index(from));
    beacon_->manager.AddShares(from, shares);
    shares_received_.insert(from);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " received duplicate shares from node ", beacon_->manager.cabinet_index(from));
  }
}

/**
 * Handler for broadcasted coefficients
 *
 * @param msg_ptr Pointer of CoefficientsMessage
 * @param from_id Muddle address of sender
 */
void BeaconSetupService::OnNewCoefficients(CoefficientsMessage const &msg,
                                           MuddleAddress const &      from)
{
  if (msg.phase() == static_cast<uint64_t>(State::WAIT_FOR_SHARE))
  {
    if (coefficients_received_.find(from) == coefficients_received_.end())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                     " received coefficients from node  ", beacon_->manager.cabinet_index(from));
      beacon_->manager.AddCoefficients(from, msg.coefficients());
      coefficients_received_.insert(from);
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                     " received duplicate coefficients from node ",
                     beacon_->manager.cabinet_index(from));
    }
  }
  else if (msg.phase() == static_cast<uint64_t>(State::WAIT_FOR_QUAL_SHARES))
  {
    if (qual_coefficients_received_.find(from) == qual_coefficients_received_.end())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                     " received qual coefficients from node  ",
                     beacon_->manager.cabinet_index(from));
      beacon_->manager.AddQualCoefficients(from, msg.coefficients());
      qual_coefficients_received_.insert(from);
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                     " received duplicate qual coefficients from node ",
                     beacon_->manager.cabinet_index(from));
    }
  }
}

/**
 * Handler for complaints message
 *
 * @param msg_ptr Pointer of ComplaintsMessage
 * @param from_id Muddle address of sender
 */
void BeaconSetupService::OnComplaints(ComplaintsMessage const &msg, MuddleAddress const &from)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                 " received complaints from node ", beacon_->manager.cabinet_index(from));
  complaints_manager_.Add(msg, from, identity_.identifier());
}

/**
 * Handler for complaints answer message containing the pairs of secret shares the sender sent to
 * members that complained against the sender
 *
 * @param msg_ptr Pointer of SharesMessage containing the sender's shares
 * @param from_id Muddle address of sender
 */
void BeaconSetupService::OnComplaintsAnswer(SharesMessage const &answer, MuddleAddress const &from)
{
  if (complaints_answer_manager_.Count(from))
  {
    CheckComplaintAnswer(answer, from);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " received multiple complaint answer from node ",
                   beacon_->manager.cabinet_index(from));
  }
}

/**
 * Handler for qual complaints message which contains the secret shares sender received from
 * members in qual complaints
 *
 * @param shares_ptr Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void BeaconSetupService::OnQualComplaints(SharesMessage const &shares_msg,
                                          MuddleAddress const &from)
{
  qual_complaints_manager_.Received(from, shares_msg.shares());
}

/**
 * Handler for messages containing secret shares of qual members that other qual members have
 * complained against
 *
 * @param shares_ptr Pointer of SharesMessage
 * @param from_id Muddle address of sender
 */
void BeaconSetupService::OnReconstructionShares(SharesMessage const &shares_msg,
                                                MuddleAddress const &from)
{
  if (reconstruction_shares_received_.find(from) == reconstruction_shares_received_.end())
  {
    reconstruction_shares_received_.insert({from, shares_msg.shares()});
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " received duplicate reconstruction shares from node ",
                   beacon_->manager.cabinet_index(from));
  }
}

/**
 * For all complaint answers received in defense of a complaint we check the exposed secret share
 * is consistent with the broadcasted coefficients
 *
 * @param answer Pointer of message containing some secret shares of sender
 * @param from_id Muddle address of sender
 * @param from_index Index of sender in cabinet_
 */
void BeaconSetupService::CheckComplaintAnswer(SharesMessage const &answer,
                                              MuddleAddress const &from_id)
{
  // If not enough answers are sent for number of complaints against a node then add a complaint a
  // against it
  auto diff = complaints_manager_.ComplaintsCount(from_id) - answer.shares().size();
  if (diff > 0 && diff <= beacon_->aeon.members.size())
  {
    complaints_answer_manager_.Add(from_id);
  }
  for (auto const &share : answer.shares())
  {
    if (!beacon_->manager.VerifyComplaintAnswer(from_id, share))
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
bool BeaconSetupService::BuildQual()
{
  // Create set of muddle addresses
  std::set<MuddleAddress> cabinet;
  for (auto &m : beacon_->aeon.members)
  {
    cabinet.insert(m.identifier());
  }
  beacon_->manager.SetQual(complaints_answer_manager_.BuildQual(cabinet));
  std::set<MuddleAddress> qual = beacon_->manager.qual();
  if (qual.find(identity_.identifier()) == qual.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                   " build QUAL failed as not in QUAL");
    return false;
  }
  else if (qual.size() <= beacon_->manager.polynomial_degree())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                   " build QUAL failed as size ", qual.size(), " less than threshold ",
                   beacon_->manager.polynomial_degree());
    return false;
  }
  return true;
}

/**
 * Checks the complaints set by qual members
 */
void BeaconSetupService::CheckQualComplaints()
{
  std::set<MuddleAddress> qual{beacon_->manager.qual()};
  for (const auto &complaint : qual_complaints_manager_.ComplaintsReceived())
  {
    MuddleAddress sender = complaint.first;
    // Return if the sender not in QUAL
    if (qual.find(sender) == qual.end())
    {
      return;
    }
    for (auto const &share : complaint.second)
    {
      // Check person who's shares are being exposed is not in QUAL then don't bother with checks
      if (qual.find(share.first) != qual.end())
      {
        qual_complaints_manager_.Complaints(beacon_->manager.VerifyQualComplaint(sender, share));
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
bool BeaconSetupService::BasicMsgCheck(MuddleAddress const &              from,
                                       std::shared_ptr<DKGMessage> const &msg_ptr)
{
  // Check if sender is in cabinet
  bool in_cabinet{false};
  for (auto &m : beacon_->aeon.members)
  {
    if (m.identifier() == from)
    {
      in_cabinet = true;
    }
  }

  if (msg_ptr == nullptr)
  {
    return false;
  }
  else if (!in_cabinet)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " received message from unknown sender");
    return false;
  }
  return true;
}

void BeaconSetupService::QueueSetup(SharedAeonExecutionUnit beacon)
{
  std::lock_guard<std::mutex> lock(mutex_);
  assert(beacon != nullptr);

  aeon_exe_queue_.push_back(beacon);
}

void BeaconSetupService::SetBeaconReadyCallback(CallbackFunction callback)
{
  std::lock_guard<std::mutex> lock(mutex_);
  callback_function_ = callback;
}

std::weak_ptr<core::Runnable> BeaconSetupService::GetWeakRunnable()
{
  return state_machine_;
}

}  // namespace beacon
}  // namespace fetch
