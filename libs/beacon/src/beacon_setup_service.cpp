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
#include "core/containers/set_difference.hpp"
#include "core/containers/set_intersection.hpp"
#include "ledger/shards/shard_management_service.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

#include <ctime>
#include <mutex>

namespace fetch {
namespace beacon {

using ledger::ServiceIdentifier;

char const *ToString(BeaconSetupService::State state)
{
  char const *text = "unknown";

  switch (state)
  {
  case BeaconSetupService::State::IDLE:
    text = "Idle";
    break;
  case BeaconSetupService::State::RESET:
    text = "+++ Reset +++";
    break;
  case BeaconSetupService::State::CONNECT_TO_ALL:
    text = "Connect to the necessary cabinet members";
    break;
  case BeaconSetupService::State::WAIT_FOR_READY_CONNECTIONS:
    text = "Waiting for ready connections";
    break;
  case BeaconSetupService::State::WAIT_FOR_SHARES:
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
  case BeaconSetupService::State::DRY_RUN_SIGNING:
    text = "Dry run of signing a seed value";
    break;
  case BeaconSetupService::State::BEACON_READY:
    text = "Beacon ready";
  }

  return text;
}

uint64_t GetTime()
{
  return static_cast<uint64_t>(std::time(nullptr));
}

BeaconSetupService::BeaconSetupService(MuddleInterface &muddle, Identity identity,
                                       ManifestCacheInterface &manifest_cache)
  : identity_{std::move(identity)}
  , manifest_cache_{manifest_cache}
  , muddle_{muddle}
  , endpoint_{muddle_.GetEndpoint()}
  , shares_subscription_(endpoint_.Subscribe(SERVICE_DKG, CHANNEL_SECRET_KEY))
  , dry_run_subscription_(endpoint_.Subscribe(SERVICE_DKG, CHANNEL_SIGN_DRY_RUN))
  , pre_dkg_rbc_{endpoint_, identity_.identifier(),
                 [this](MuddleAddress const &from, ConstByteArray const &payload) -> void {
                   std::set<MuddleAddress>               connections;
                   fetch::serializers::MsgPackSerializer serializer{payload};
                   serializer >> connections;

                   std::lock_guard<std::mutex> lock(mutex_);
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
  , beacon_dkg_state_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_state_gauge", "State the DKG is in as integer in [0, 10]")}
  , beacon_dkg_connections_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_connections_gauge", "Connections the network has made as a prerequisite")}
  , beacon_dkg_all_connections_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_all_connections_gauge", "Connections the network has made in general")}
  , beacon_dkg_failures_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_dkg_failures_total", "The total number of DKG failures")}
  , beacon_dkg_dry_run_failures_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_dkg_dry_run_failures_total", "The total number of DKG dry run failures")}
{
  // clang-format off
  state_machine_->RegisterHandler(State::IDLE, this, &BeaconSetupService::OnIdle);
  state_machine_->RegisterHandler(State::RESET, this, &BeaconSetupService::OnReset);
  state_machine_->RegisterHandler(State::CONNECT_TO_ALL, this, &BeaconSetupService::OnConnectToAll);
  state_machine_->RegisterHandler(State::WAIT_FOR_READY_CONNECTIONS, this, &BeaconSetupService::OnWaitForReadyConnections);
  state_machine_->RegisterHandler(State::WAIT_FOR_SHARES, this, &BeaconSetupService::OnWaitForShares);
  state_machine_->RegisterHandler(State::WAIT_FOR_COMPLAINTS, this, &BeaconSetupService::OnWaitForComplaints);
  state_machine_->RegisterHandler(State::WAIT_FOR_COMPLAINT_ANSWERS, this, &BeaconSetupService::OnWaitForComplaintAnswers);
  state_machine_->RegisterHandler(State::WAIT_FOR_QUAL_SHARES, this, &BeaconSetupService::OnWaitForQualShares);
  state_machine_->RegisterHandler(State::WAIT_FOR_QUAL_COMPLAINTS, this, &BeaconSetupService::OnWaitForQualComplaints);
  state_machine_->RegisterHandler(State::WAIT_FOR_RECONSTRUCTION_SHARES, this, &BeaconSetupService::OnWaitForReconstructionShares);
  state_machine_->RegisterHandler(State::DRY_RUN_SIGNING, this, &BeaconSetupService::OnDryRun);
  state_machine_->RegisterHandler(State::BEACON_READY, this, &BeaconSetupService::OnBeaconReady);
  // clang-format on

  // Set subscription for receiving shares
  shares_subscription_->SetMessageHandler(this, &BeaconSetupService::OnNewSharesPacket);
  dry_run_subscription_->SetMessageHandler(this, &BeaconSetupService::OnNewDryRunPacket);
}

BeaconSetupService::State BeaconSetupService::OnIdle()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::IDLE));

  beacon_.reset();

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
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(100));
  return State::IDLE;
}

/**
 * Reset and initial state for the DKG. It should return to this state in the
 * case of total DKG failure
 *
 */
BeaconSetupService::State BeaconSetupService::OnReset()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::RESET));

  if (state_machine_->previous_state() != State::RESET &&
      state_machine_->previous_state() != State::IDLE)
  {
    beacon_dkg_failures_total_->add(1u);
  }

  assert(beacon_);

  // Initiating setup
  std::set<MuddleAddress> cabinet;
  for (auto &m : beacon_->aeon.members)
  {
    cabinet.insert(m.identifier());
  }

  auto cabinet_size = static_cast<uint32_t>(cabinet.size());

  coefficients_received_.clear();
  complaints_answer_manager_.ResetCabinet(cabinet_size);
  complaints_manager_.ResetCabinet(cabinet_size);
  connections_.clear();
  pre_dkg_rbc_.ResetCabinet(cabinet);
  qual_coefficients_received_.clear();
  qual_complaints_manager_.Reset();
  rbc_.ResetCabinet(cabinet);
  ready_connections_.clear();
  reconstruction_shares_received_.clear();
  shares_received_.clear();
  dry_run_shares_.clear();
  dry_run_public_keys_.clear();

  if (beacon_->aeon.round_start < abort_below_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Aborting DKG");
    return State::IDLE;
  }

  // The pre-dkg will get held in reset mode until the DKG start time
  if (timer_to_proceed_.HasExpired())
  {
    SetTimeToProceed(State::CONNECT_TO_ALL);
    return State::CONNECT_TO_ALL;
  }

  return State::RESET;
}

/**
 * Tell the muddle network to directly connect to the cabinet members
 * for this aeon
 *
 */
BeaconSetupService::State BeaconSetupService::OnConnectToAll()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::CONNECT_TO_ALL));

  if (state_machine_->previous_state() != State::CONNECT_TO_ALL)
  {
    std::unordered_set<MuddleAddress> aeon_members;
    for (auto &m : beacon_->aeon.members)
    {
      // Skipping own address
      if (m == identity_)
      {
        continue;
      }

      aeon_members.emplace(m.identifier());
    }

    // add the outstanding peers
    auto const connected_peers   = muddle_.GetDirectlyConnectedPeers();
    auto const outstanding_peers = aeon_members - connected_peers;

    ledger::Manifest manifest{};
    for (auto const &address : outstanding_peers)
    {
      std::unique_ptr<network::Uri> hint{};

      // lookup the manifest for the desired address
      if (manifest_cache_.QueryManifest(address, manifest))
      {
        // attempt to find the service entry
        auto it = manifest.FindService(ServiceIdentifier::Type::DKG);
        if (it != manifest.end())
        {
          hint = std::make_unique<network::Uri>(it->second.uri());
        }
      }

      if (hint)
      {
        // tell muddle to connect to the address with the specified hint
        muddle_.ConnectTo(address, *hint);
      }
      else
      {
        // tell muddle to connect to the address using normal service discovery
        muddle_.ConnectTo(address);
      }
    }

    // request removal of unwanted connections
    auto unwanted_connections = muddle_.GetRequestedPeers() - aeon_members;
    FETCH_LOG_INFO(LOGGING_NAME, "Removing unwanted connections: ", unwanted_connections.size());
    muddle_.DisconnectFrom(unwanted_connections);
  }

  if (timer_to_proceed_.HasExpired())
  {
    SetTimeToProceed(State::WAIT_FOR_READY_CONNECTIONS);
    return State::WAIT_FOR_READY_CONNECTIONS;
  }

  return State::CONNECT_TO_ALL;
}

// Helper function to compute the required connected peers
uint64_t BeaconSetupService::PreDKGThreshold()
{
  uint64_t cabinet_size = beacon_->aeon.members.size();
  uint64_t threshold    = beacon_->manager.polynomial_degree() + 1;

  uint64_t ret = threshold + (cabinet_size / 3);

  // Needs at least two members to be distributed
  assert(ret >= 2);

  return ret;
}

template <typename T>
std::set<T> ConvertToSet(std::unordered_set<T> const &from)
{
  std::set<T> ret;

  for (auto const &i : from)
  {
    ret.insert(i);
  }

  return ret;
}

/**
 * Wait until threshold members have connected to the network. This
 * is the only blocking state in the DKG.
 *
 */
BeaconSetupService::State BeaconSetupService::OnWaitForReadyConnections()
{
  std::lock_guard<std::mutex> lock(mutex_);

  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_READY_CONNECTIONS));

  auto const connected_peers = muddle_.GetDirectlyConnectedPeers();

  std::unordered_set<MuddleAddress> aeon_members;
  for (auto &m : beacon_->aeon.members)
  {
    // Skipping own address
    if (m == identity_)
    {
      continue;
    }

    aeon_members.emplace(m.identifier());
  }

  auto       can_see             = (connected_peers & aeon_members);
  auto const require_connections = PreDKGThreshold() - 1;

  FETCH_LOG_DEBUG(LOGGING_NAME, "All connections:       ", connected_peers.size());
  FETCH_LOG_DEBUG(LOGGING_NAME, "Relevant connections:  ", can_see.size());

  beacon_dkg_all_connections_gauge_->set(connected_peers.size());
  beacon_dkg_connections_gauge_->set(can_see.size());

  // If we get over threshold connections, send a message to all peers each time
  // we increase over this threshold (note we won't advance if we ourselves don't get over)
  if (can_see.size() > connections_.size() && can_see.size() >= require_connections)
  {
    fetch::serializers::MsgPackSerializer serializer;
    serializer << connections_;
    pre_dkg_rbc_.Broadcast(serializer.data());

    FETCH_LOG_INFO(LOGGING_NAME, "Minimum peer threshold requirement met for DKG");

    connections_ = ConvertToSet(can_see);
  }

  // Whether to proceed (if threshold peers have also met this condition)
  const bool is_ok = (ready_connections_.size() >= require_connections &&
                      connections_.size() >= require_connections);

  if (!condition_to_proceed_ && is_ok)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(), " of ",
                   seconds_for_state_);
  }

  if (timer_to_proceed_.HasExpired())
  {
    if (!condition_to_proceed_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to guarantee peers were ready for DKG!");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }

    BroadcastShares();
    SetTimeToProceed(State::WAIT_FOR_SHARES);
    return State::WAIT_FOR_SHARES;
  }
  else
  {
    if (!condition_to_proceed_)
    {
      FETCH_LOG_INFO(
          LOGGING_NAME,
          "Waiting for all peers to be ready before starting DKG. We have: ", can_see.size(),
          " expect: ", require_connections, " Other ready peers: ", ready_connections_.size());
    }

    state_machine_->Delay(std::chrono::milliseconds(500));
  }

  return State::WAIT_FOR_READY_CONNECTIONS;
}

/**
 * The node has broadcast it's own shares at this point, now wait
 * to receive shares from everyone else
 *
 */
BeaconSetupService::State BeaconSetupService::OnWaitForShares()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_SHARES));

  auto       intersection = (coefficients_received_ & shares_received_);
  const bool is_ok        = intersection.size() >= beacon_->manager.polynomial_degree();

  if (!condition_to_proceed_ && is_ok)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(), " of ",
                   seconds_for_state_);
  }

  if (timer_to_proceed_.HasExpired())
  {
    if (!condition_to_proceed_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to collect enough shares! Resetting");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }

    BroadcastComplaints();
    SetTimeToProceed(State::WAIT_FOR_COMPLAINTS);
    return State::WAIT_FOR_COMPLAINTS;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnWaitForComplaints()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_COMPLAINTS));

  const bool is_ok = complaints_manager_.IsFinished(beacon_->manager.polynomial_degree());

  if (!condition_to_proceed_ && is_ok)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(), " of ",
                   seconds_for_state_);
  }

  if (timer_to_proceed_.HasExpired())
  {
    if (!condition_to_proceed_)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Failed to collect enough complaints within the time period! Resetting.");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }

    // Complaints at this point consist only of parties which have received over threshold number of
    // complaints
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), " complaints size ",
                   complaints_manager_.Complaints().size());
    complaints_answer_manager_.Init(complaints_manager_.Complaints());

    BroadcastComplaintsAnswer();
    SetTimeToProceed(State::WAIT_FOR_COMPLAINT_ANSWERS);
    return State::WAIT_FOR_COMPLAINT_ANSWERS;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_COMPLAINTS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForComplaintAnswers()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS));

  // TODO(HUT): fix this.
  // bool const is_ok = complaints_answer_manager_.IsFinished();
  bool const is_ok = true;

  if (!condition_to_proceed_ && is_ok)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(), " of ",
                   seconds_for_state_);
  }

  if (timer_to_proceed_.HasExpired())
  {
    if (!is_ok)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Ran out of time to collect complaint answers! Resetting.");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }

    if (BuildQual())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), "build qual size ",
                     beacon_->manager.qual().size());
      beacon_->manager.ComputeSecretShare();
      BroadcastQualCoefficients();

      SetTimeToProceed(State::WAIT_FOR_QUAL_SHARES);
      return State::WAIT_FOR_QUAL_SHARES;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to build qualified set! Resetting.");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_COMPLAINT_ANSWERS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForQualShares()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_QUAL_SHARES));

  auto       intersection = (qual_coefficients_received_ & beacon_->manager.qual());
  const bool is_ok        = (intersection.size() >= beacon_->manager.polynomial_degree());

  if (timer_to_proceed_.HasExpired())
  {
    if (!is_ok)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Failed to collect enough qual shares! Collected: ",
                     qual_coefficients_received_.size(), " of ", beacon_->manager.qual().size());
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }

    BroadcastQualComplaints();

    SetTimeToProceed(State::WAIT_FOR_QUAL_COMPLAINTS);
    return State::WAIT_FOR_QUAL_COMPLAINTS;
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_QUAL_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnWaitForQualComplaints()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS));

  const bool is_ok =
      /* qual_complaints_manager_.IsFinished(beacon_->manager.qual(), identity_.identifier());*/
      true;
  condition_to_proceed_ = is_ok;

  if (timer_to_proceed_.HasExpired())
  {
    if (!condition_to_proceed_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Failed to collect enough qual complaints!");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }

    CheckQualComplaints();
    std::size_t const size = qual_complaints_manager_.ComplaintsSize();

    if (size > beacon_->manager.polynomial_degree())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                     " DKG has failed: complaints size ", size, " greater than threshold.");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }
    else if (qual_complaints_manager_.ComplaintsFind(identity_.identifier()))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                     " is in qual complaints");
      beacon_->manager.ComputePublicKeys();
    }
    BroadcastReconstructionShares();

    SetTimeToProceed(State::WAIT_FOR_RECONSTRUCTION_SHARES);
    return State::WAIT_FOR_RECONSTRUCTION_SHARES;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_QUAL_COMPLAINTS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForReconstructionShares()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES));

  MuddleAddresses complaints_list = qual_complaints_manager_.Complaints();
  MuddleAddresses remaining_honest;
  MuddleAddresses qual{beacon_->manager.qual()};
  std::set_difference(qual.begin(), qual.end(), complaints_list.begin(), complaints_list.end(),
                      std::inserter(remaining_honest, remaining_honest.begin()));

  uint16_t received_count = 0;
  for (auto const &member : remaining_honest)
  {
    if (member != identity_.identifier() &&
        reconstruction_shares_received_.find(member) != reconstruction_shares_received_.end())
    {
      received_count++;
    }
  }

  if (timer_to_proceed_.HasExpired())
  {
    // Process reconstruction shares. Reconstruction shares from non-qual members
    // or people in qual complaints should not be considered
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

    if (!beacon_->manager.RunReconstruction())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                     " DKG failed due to reconstruction failure. Resetting.");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }
    else
    {
      beacon_->manager.ComputePublicKeys();
    }

    SetTimeToProceed(State::DRY_RUN_SIGNING);
    return State::DRY_RUN_SIGNING;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_RECONSTRUCTION_SHARES;
}

/**
 * Attempt to sign the seed to determine enough peers have generated the
 * group public signature
 */
BeaconSetupService::State BeaconSetupService::OnDryRun()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Only on first entry to this function
  if (state_machine_->previous_state() != State::DRY_RUN_SIGNING)
  {
    beacon_->manager.SetMessage("test message");
    beacon_->member_share = beacon_->manager.Sign();

    // insert ourselves - others will insert here also via gossip
    dry_run_shares_[identity_.identifier()] = beacon_->member_share;

    DryRunInfo to_send{beacon_->manager.group_public_key(), beacon_->member_share};

    // Gossip this to everyone
    // TODO(HUT): size of member share is known so can reserve.
    {
      // serializer.Reserve(counter.size());
      fetch::serializers::MsgPackSerializer serializer;
      serializer << to_send;
      FETCH_LOG_INFO(LOGGING_NAME, "share size: ", serializer.size());
      endpoint_.Broadcast(SERVICE_DKG, CHANNEL_SIGN_DRY_RUN, serializer.data());
    }
  }

  if (timer_to_proceed_.HasExpired())
  {
    bool found_key = false;

    for (auto const &key_and_count : dry_run_public_keys_)
    {
      if (key_and_count.second >= beacon_->manager.polynomial_degree())
      {
        found_key = true;
      }
    }

    if (!found_key)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to reach consensus on group public key!");
    }

    for (auto const &share : dry_run_shares_)
    {
      beacon_->manager.AddSignaturePart(share.second.identity, share.second.signature);
    }

    bool const could_sign = beacon_->manager.can_verify() && beacon_->manager.Verify();

    if (!could_sign)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to sign group signature");
    }

    if (could_sign && found_key)
    {
      SetTimeToProceed(State::BEACON_READY);
      return State::BEACON_READY;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to complete dry run for signature signing!");
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }
  }

  return State::DRY_RUN_SIGNING;
}

BeaconSetupService::State BeaconSetupService::OnBeaconReady()
{
  std::lock_guard<std::mutex> lock(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::BEACON_READY));

  FETCH_LOG_INFO(LOGGING_NAME, "*** New beacon generated! ***");

  if (callback_function_)
  {
    callback_function_(beacon_);
  }

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
  SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES),
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
    endpoint_.Send(cab_i.identifier(), SERVICE_DKG, CHANNEL_SECRET_KEY, serializer.data(),
                   MuddleEndpoint::OPTION_ENCRYPTED);
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
  qual_coefficients_received_.insert(identity_.identifier());
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
  FETCH_UNUSED(from_index);

  if (phase1 == static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                    " received complaint answer from ", from_index);
    OnComplaintsAnswer(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                    " received QUAL complaint from ", from_index);
    OnQualComplaints(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node: ", beacon_->manager.cabinet_index(),
                    " received reconstruction share from ", from_index);
    OnReconstructionShares(shares, from_id);
  }
}

void BeaconSetupService::OnNewSharesPacket(muddle::Packet const &packet,
                                           MuddleAddress const & last_hop)
{
  FETCH_UNUSED(last_hop);

  // // TODO(EJF): This will need to be enabled after encryption support has been added
#if 0
  if (!packet.IsEncrypted())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Non encrpypted packet recv'ed");
    return;
  }
#endif

  fetch::serializers::MsgPackSerializer serialiser(packet.GetPayload());

  std::pair<std::string, std::string> shares;
  serialiser >> shares;

  // Dispatch the event
  OnNewShares(packet.GetSender(), shares);
}

void BeaconSetupService::OnNewDryRunPacket(muddle::Packet const &packet,
                                           MuddleAddress const & last_hop)
{
  FETCH_UNUSED(last_hop);

  fetch::serializers::MsgPackSerializer serialiser(packet.GetPayload());

  DryRunInfo to_receive;
  serialiser >> to_receive;

  // TODO(HUT): cabinet check
  std::lock_guard<std::mutex> lock(mutex_);
  dry_run_public_keys_[to_receive.public_key]++;
  dry_run_shares_[packet.GetSender()] = to_receive.sig_share;
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
  std::lock_guard<std::mutex> lock(mutex_);
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
  if (msg.phase() == static_cast<uint64_t>(State::WAIT_FOR_SHARES))
  {
    if (coefficients_received_.find(from) == coefficients_received_.end())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
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
  FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
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

void BeaconSetupService::Abort(uint64_t abort_below)
{
  std::lock_guard<std::mutex> lock(mutex_);
  abort_below_ = abort_below;
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

uint64_t GetExpectedDKGTime(uint64_t cabinet_size)
{
  // Default
  uint64_t expected_dkg_time_s = 10 * (cabinet_size << 1);

  // empirical times observed - the base time you expect it to take based on cabinet size
  if (cabinet_size < 200)
  {
    expected_dkg_time_s = 27229;
  }
  if (cabinet_size < 90)
  {
    expected_dkg_time_s = 1304;
  }
  if (cabinet_size < 60)
  {
    expected_dkg_time_s = 305;
  }
  if (cabinet_size < 30)
  {
    expected_dkg_time_s = 100;
  }
  if (cabinet_size < 10)
  {
    expected_dkg_time_s = 30;
  }

  return expected_dkg_time_s;
}

void SetTimeBySlots(BeaconSetupService::State state, uint64_t &time_slots_total,
                    uint64_t &time_slot_for_state)
{
  std::map<BeaconSetupService::State, uint64_t> time_slot_map;

  time_slot_map[BeaconSetupService::State::RESET]                          = 0;
  time_slot_map[BeaconSetupService::State::CONNECT_TO_ALL]                 = 10;
  time_slot_map[BeaconSetupService::State::WAIT_FOR_READY_CONNECTIONS]     = 10;
  time_slot_map[BeaconSetupService::State::WAIT_FOR_SHARES]                = 10;
  time_slot_map[BeaconSetupService::State::WAIT_FOR_COMPLAINTS]            = 10;
  time_slot_map[BeaconSetupService::State::WAIT_FOR_COMPLAINT_ANSWERS]     = 10;
  time_slot_map[BeaconSetupService::State::WAIT_FOR_QUAL_SHARES]           = 10;
  time_slot_map[BeaconSetupService::State::WAIT_FOR_QUAL_COMPLAINTS]       = 10;
  time_slot_map[BeaconSetupService::State::WAIT_FOR_RECONSTRUCTION_SHARES] = 10;
  time_slot_map[BeaconSetupService::State::DRY_RUN_SIGNING]                = 20;

  time_slot_for_state = time_slot_map[state];

  while (state != BeaconSetupService::State::RESET)
  {
    time_slots_total += time_slot_map[state];
    state = BeaconSetupService::State(static_cast<uint8_t>(state) - 1);
  }
}

/**
 * Set the time to proceed to the next state given
 * that we are entering the State 'state'. The function will set a timer
 * that will expire when it is time to move to the next state.
 *
 */
void BeaconSetupService::SetTimeToProceed(BeaconSetupService::State state)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Determining time allowed to move on from state: ", ToString(state));
  condition_to_proceed_ = false;

  uint64_t cabinet_size        = beacon_->aeon.members.size();
  uint64_t expected_dkg_time_s = GetExpectedDKGTime(cabinet_size);
  uint64_t current_time        = GetTime();

  // RESET state will delay DKG until the start point (or next start point)
  if (state == BeaconSetupService::State::RESET)
  {
    // Easy case where the start point is ahead in time
    // If not ahead in time, the DKG must have failed before. Algorithmically decide how long
    // to increase the allotted DKG time (scheme 2x)
    uint64_t next_start_point = beacon_->aeon.start_reference_timepoint;
    uint64_t dkg_time         = expected_dkg_time_s;
    uint16_t failures         = 0;

    while (next_start_point < current_time)
    {
      failures++;
      next_start_point += dkg_time;
      dkg_time = dkg_time + uint64_t(0.5 * expected_dkg_time_s);
    }

    expected_dkg_timespan_ = dkg_time;
    reference_timepoint_   = next_start_point;

    FETCH_LOG_INFO(LOGGING_NAME, "DKG: ", beacon_->aeon.round_start, " failures so far: ", failures,
                   " allotted time: ", expected_dkg_timespan_, " base time: ", expected_dkg_time_s);
  }

  // No timeout for these states
  if (state == BeaconSetupService::State::BEACON_READY || state == BeaconSetupService::State::IDLE)
  {
    return;
  }
  // Assign each state an equal amount of time for now
  uint64_t time_slots_in_dkg   = 100;
  double   time_per_slot       = double(expected_dkg_timespan_) / double(time_slots_in_dkg);
  uint64_t time_slots_total    = 0;
  uint64_t time_slot_for_state = 0;

  SetTimeBySlots(state, time_slots_total, time_slot_for_state);

  seconds_for_state_ = uint64_t(time_slot_for_state * time_per_slot);
  state_deadline_    = reference_timepoint_ + uint64_t(time_slots_total * time_per_slot);

  if (state_deadline_ < current_time)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "\n#### Deadline for ", ToString(state),
                   " has passed! This should not happen");
    timer_to_proceed_.Restart(0);
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "#### Proceeding to next state \"", ToString(state),
                   "\", to last ", state_deadline_ - current_time,
                   " seconds (deadline: ", state_deadline_, ")");
    timer_to_proceed_.Restart(std::chrono::seconds{state_deadline_ - current_time});
  }
}

}  // namespace beacon

}  // namespace fetch
