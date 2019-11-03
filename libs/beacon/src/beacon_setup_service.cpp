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
#include "beacon/block_entropy.hpp"
#include "core/containers/set_difference.hpp"
#include "core/containers/set_intersection.hpp"
#include "crypto/verifier.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "shards/shard_management_service.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

#include <ctime>
#include <mutex>
#include <utility>

namespace fetch {
namespace beacon {

using shards::ServiceIdentifier;

char const *ToString(BeaconSetupService::State state);

// Convenience factory to set up the RBC/PBC
BeaconSetupService::ReliableChannelPtr BeaconSetupService::ReliableBroadcastFactory()
{
  using ChannelType = muddle::RBC;
  // using ChannelType = PunishmentBroadcastChannel; // The other option

  auto call_on_msg = [this](MuddleAddress const &from, ConstByteArray const &payload) -> void {
    DKGEnvelope   env;
    DKGSerializer serializer{payload};
    serializer >> env;
    OnDkgMessage(from, env.Message());
  };

  return std::make_unique<ChannelType>(endpoint_, identity_.identifier(), call_on_msg, certificate_,
                                       CHANNEL_RBC_BROADCAST, false);
}

BeaconSetupService::BeaconSetupService(MuddleInterface &muddle, Identity identity,
                                       ManifestCacheInterface &manifest_cache,
                                       CertificatePtr          certificate)
  : identity_{std::move(identity)}
  , manifest_cache_{manifest_cache}
  , muddle_{muddle}
  , endpoint_{muddle_.GetEndpoint()}
  , shares_subscription_(endpoint_.Subscribe(SERVICE_DKG, CHANNEL_SECRET_KEY))
  , certificate_{std::move(certificate)}
  , rbc_{ReliableBroadcastFactory()}
  , state_machine_{std::make_shared<StateMachine>("BeaconSetupService", State::IDLE, ToString)}
  , beacon_dkg_state_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_state_gauge", "State the DKG is in as integer in [0, 10]")}
  , beacon_dkg_connections_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_connections_gauge", "Connections the network has made as a prerequisite")}
  , beacon_dkg_all_connections_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_all_connections_gauge", "Connections the network has made in general")}
  , beacon_dkg_failures_required_to_complete_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_failures_required_to_complete", "Failures before the DKG was successful")}
  , beacon_dkg_state_failed_on_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_state_failed_on", "Last state the DKG failed on")}
  , beacon_dkg_time_allocated_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_time_allocated", "Time allocated for the DKG to complete")}
  , beacon_dkg_aeon_setting_up_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_aeon_setting_up", "The aeon currently under setup.")}
  , beacon_dkg_miners_in_qual_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_dkg_miners_in_qual", "Number of miners that have made it into qual")}
  , beacon_dkg_failures_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_dkg_failures_total", "The total number of DKG failures")}
  , beacon_dkg_aborts_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_dkg_aborts_total", "The total number of DKG forced aborts")}
  , beacon_dkg_successes_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_dkg_successes_total", "The total number of DKG successes")}
  , time_slot_map_{{BeaconSetupService::State::RESET, 0},
                   {BeaconSetupService::State::CONNECT_TO_ALL, 10},
                   {BeaconSetupService::State::WAIT_FOR_READY_CONNECTIONS, 10},
                   {BeaconSetupService::State::WAIT_FOR_SHARES, 10},
                   {BeaconSetupService::State::WAIT_FOR_COMPLAINTS, 10},
                   {BeaconSetupService::State::WAIT_FOR_COMPLAINT_ANSWERS, 10},
                   {BeaconSetupService::State::WAIT_FOR_QUAL_SHARES, 10},
                   {BeaconSetupService::State::WAIT_FOR_QUAL_COMPLAINTS, 10},
                   {BeaconSetupService::State::WAIT_FOR_RECONSTRUCTION_SHARES, 10},
                   {BeaconSetupService::State::COMPUTE_PUBLIC_SIGNATURE, 10},
                   {BeaconSetupService::State::DRY_RUN_SIGNING, 10}}
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
  state_machine_->RegisterHandler(State::COMPUTE_PUBLIC_SIGNATURE, this, &BeaconSetupService::OnComputePublicSignature);
  state_machine_->RegisterHandler(State::DRY_RUN_SIGNING, this, &BeaconSetupService::OnDryRun);
  state_machine_->RegisterHandler(State::BEACON_READY, this, &BeaconSetupService::OnBeaconReady);
  // clang-format on

  // Set subscription for receiving shares
  shares_subscription_->SetMessageHandler(this, &BeaconSetupService::OnNewSharesPacket);

  for (auto &slot : time_slot_map_)
  {
    time_slots_in_dkg_ += slot.second;
  }
}

BeaconSetupService::State BeaconSetupService::OnIdle()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::IDLE));
  beacon_dkg_all_connections_gauge_->set(muddle_.GetDirectlyConnectedPeers().size());
  beacon_dkg_aeon_setting_up_->set(0);

  beacon_.reset();

  if (!aeon_exe_queue_.empty())
  {
    beacon_ = aeon_exe_queue_.front();
    assert(beacon_ != nullptr);

    aeon_exe_queue_.pop_front();

    SetTimeToProceed(State::RESET);
    return State::RESET;
  }

  state_machine_->Delay(std::chrono::milliseconds(100));
  return State::IDLE;
}

/**
 * Reset and initial state for the DKG. It should return to this state in the
 * case of total DKG failure
 */
BeaconSetupService::State BeaconSetupService::OnReset()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::RESET));
  beacon_dkg_all_connections_gauge_->set(muddle_.GetDirectlyConnectedPeers().size());
  beacon_dkg_aeon_setting_up_->set(beacon_->aeon.round_start);

  if (state_machine_->previous_state() != State::RESET &&
      state_machine_->previous_state() != State::IDLE)
  {
    beacon_dkg_failures_total_->add(1u);
  }

  // TODO(HUT): need to look at this when merged with Ed's changes for pre aeon setup.
  bool const beacon_outdated = beacon_->aeon.round_start < abort_below_;
  bool const beacon_updated =
      (!aeon_exe_queue_.empty() &&
       aeon_exe_queue_.front()->aeon.round_start == beacon_->aeon.round_start);

  // abort if the current beacon is out of date
  if (beacon_outdated || beacon_updated)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Aborting DKG. Round start: ", beacon_->aeon.round_start,
                   " abort all below: ", abort_below_);
    beacon_dkg_aborts_total_->add(1u);
    return State::IDLE;
  }

  assert(beacon_);
  beacon_->manager.Reset();

  // Initiating setup
  connections_.clear();
  ready_connections_.clear();
  shares_received_.clear();
  coefficients_received_.clear();
  qual_coefficients_received_.clear();
  reconstruction_shares_received_.clear();
  complaints_manager_.ResetCabinet(identity_.identifier(),
                                   beacon_->manager.polynomial_degree() + 1);
  complaint_answers_manager_.ResetCabinet();
  qual_complaints_manager_.Reset();
  final_state_payload_.clear();
  rbc_->Enable(false);

  // The dkg has to be reset to 0 to clear old messages,
  // before being reset with the cabinet
  if (timer_to_proceed_.HasExpired())
  {
    rbc_->Enable(true);
    rbc_->ResetCabinet(beacon_->aeon.members);

    SetTimeToProceed(State::CONNECT_TO_ALL);
    return State::CONNECT_TO_ALL;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::RESET;
}

/**
 * Tell the muddle network to directly connect to the cabinet members
 * for this aeon
 *
 */
BeaconSetupService::State BeaconSetupService::OnConnectToAll()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::CONNECT_TO_ALL));

  std::unordered_set<MuddleAddress> aeon_members;
  for (auto &member : beacon_->aeon.members)
  {
    // Skipping own address
    if (member == identity_.identifier())
    {
      continue;
    }

    aeon_members.emplace(member);
  }

  // add the outstanding peers
  auto const connected_peers   = muddle_.GetDirectlyConnectedPeers();
  auto const outstanding_peers = aeon_members - connected_peers;

  shards::Manifest manifest{};
  for (auto const &address : outstanding_peers)
  {
    std::unique_ptr<network::Uri> hint{};

    // look up the manifest for the desired address
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
  muddle_.DisconnectFrom(unwanted_connections);

  // Update telemetry
  beacon_dkg_all_connections_gauge_->set(muddle_.GetDirectlyConnectedPeers().size());

  if (timer_to_proceed_.HasExpired())
  {
    SetTimeToProceed(State::WAIT_FOR_READY_CONNECTIONS);
    return State::WAIT_FOR_READY_CONNECTIONS;
  }

  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::CONNECT_TO_ALL;
}

// Helper function to compute the required connected peers
uint64_t BeaconSetupService::PreDKGThreshold()
{
  uint64_t cabinet_size = beacon_->aeon.members.size();
  uint64_t threshold    = beacon_->manager.polynomial_degree() + 1;

  uint64_t ret = threshold + (cabinet_size / 3);

  // Needs at least two members to be distributed
  if (ret < 2)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "DKG has to few in cabinet: ", cabinet_size);
    ret = 3;
  }

  return ret;
}

uint32_t BeaconSetupService::QualSize()
{
  // Set to 2/3n for now
  auto proposed_qual_size =
      static_cast<uint32_t>(beacon_->aeon.members.size() - beacon_->aeon.members.size() / 3);
  if (proposed_qual_size <= beacon_->manager.polynomial_degree())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Default minimum qual size below threshold. Set to threshold");
    proposed_qual_size = beacon_->manager.polynomial_degree() + 1;
  }
  return proposed_qual_size;
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
  FETCH_LOCK(mutex_);

  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_READY_CONNECTIONS));

  auto const connected_peers = muddle_.GetDirectlyConnectedPeers();

  std::unordered_set<MuddleAddress> aeon_members;
  for (auto &member : beacon_->aeon.members)
  {
    // Skipping own address
    if (member == identity_.identifier())
    {
      continue;
    }

    aeon_members.emplace(member);
  }

  auto       can_see             = (connected_peers & aeon_members);
  auto const require_connections = PreDKGThreshold() - 1;

  FETCH_LOG_DEBUG(LOGGING_NAME, "All connections:       ", connected_peers.size());
  FETCH_LOG_DEBUG(LOGGING_NAME, "Relevant connections:  ", can_see.size());

  beacon_dkg_all_connections_gauge_->set(connected_peers.size());
  beacon_dkg_connections_gauge_->set(can_see.size());

  // If we get over threshold connections, send a message to all peers with the info
  // (note we won't advance if we ourselves don't get over)
  if (can_see.size() > connections_.size() && can_see.size() >= require_connections &&
      !condition_to_proceed_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " Minimum peer threshold requirement met for DKG");

    connections_ = ConvertToSet(can_see);
    SendBroadcast(DKGEnvelope{ConnectionsMessage{connections_}});
  }

  // Whether to proceed (if threshold peers have also met this condition)
  const bool is_ok = (ready_connections_.size() >= require_connections &&
                      connections_.size() >= require_connections);

  if (!condition_to_proceed_ && is_ok)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(system_clock_));
  }

  if (timer_to_proceed_.HasExpired() || condition_to_proceed_)
  {
    if (!condition_to_proceed_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                     " Failed to guarantee peers were ready for DKG!");

      beacon_dkg_state_failed_on_->set(static_cast<uint64_t>(state_machine_->state()));
      SetTimeToProceed(State::RESET);
      return State::RESET;
    }

    BroadcastShares();
    SetTimeToProceed(State::WAIT_FOR_SHARES);
    return State::WAIT_FOR_SHARES;
  }

  if (!condition_to_proceed_)
  {
    FETCH_LOG_DEBUG(
        LOGGING_NAME,
        "Waiting for all peers to be ready before starting DKG. We have: ", can_see.size(),
        " expect: ", require_connections, " Other ready peers: ", ready_connections_.size());
  }

  state_machine_->Delay(std::chrono::milliseconds(100));

  return State::WAIT_FOR_READY_CONNECTIONS;
}

/**
 * The node has broadcast it's own shares at this point, now wait
 * to receive shares from everyone else
 *
 */
BeaconSetupService::State BeaconSetupService::OnWaitForShares()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_SHARES));

  auto intersection = (coefficients_received_ & shares_received_);
  if (!condition_to_proceed_ && intersection.size() == beacon_->aeon.members.size() - 1)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(system_clock_));
  }

  if (timer_to_proceed_.HasExpired() || condition_to_proceed_)
  {
    BroadcastComplaints();
    SetTimeToProceed(State::WAIT_FOR_COMPLAINTS);
    return State::WAIT_FOR_COMPLAINTS;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnWaitForComplaints()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_COMPLAINTS));

  if (!condition_to_proceed_ &&
      complaints_manager_.NumComplaintsReceived() == beacon_->aeon.members.size() - 1)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(system_clock_));
  }

  if (timer_to_proceed_.HasExpired() || condition_to_proceed_)
  {
    complaints_manager_.Finish(beacon_->aeon.members);

    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), " complaints size ",
                    complaints_manager_.Complaints().size());
    complaint_answers_manager_.Init(complaints_manager_.Complaints());

    BroadcastComplaintAnswers();
    SetTimeToProceed(State::WAIT_FOR_COMPLAINT_ANSWERS);
    return State::WAIT_FOR_COMPLAINT_ANSWERS;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_COMPLAINTS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForComplaintAnswers()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS));

  if (!condition_to_proceed_ &&
      complaint_answers_manager_.NumComplaintAnswersReceived() == beacon_->aeon.members.size() - 1)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(system_clock_));
  }

  if (timer_to_proceed_.HasExpired() || condition_to_proceed_)
  {
    complaint_answers_manager_.Finish(beacon_->aeon.members, identity_.identifier());
    CheckComplaintAnswers();
    if (BuildQual())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), " build qual size ",
                     beacon_->manager.qual().size());
      beacon_->manager.ComputeSecretShare();
      BroadcastQualCoefficients();

      SetTimeToProceed(State::WAIT_FOR_QUAL_SHARES);
      return State::WAIT_FOR_QUAL_SHARES;
    }

    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " Failed to build qualified set! Resetting.");
    beacon_dkg_state_failed_on_->set(static_cast<uint64_t>(state_machine_->state()));
    SetTimeToProceed(State::RESET);
    return State::RESET;
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_COMPLAINT_ANSWERS;
}

BeaconSetupService::State BeaconSetupService::OnWaitForQualShares()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_QUAL_SHARES));

  auto intersection = (qual_coefficients_received_ & beacon_->manager.qual());
  if (!condition_to_proceed_ && intersection.size() == beacon_->manager.qual().size() - 1)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(system_clock_));
  }

  if (timer_to_proceed_.HasExpired() || condition_to_proceed_)
  {
    BroadcastQualComplaints();

    SetTimeToProceed(State::WAIT_FOR_QUAL_COMPLAINTS);
    return State::WAIT_FOR_QUAL_COMPLAINTS;
  }
  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_QUAL_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnWaitForQualComplaints()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS));

  if (!condition_to_proceed_ && qual_complaints_manager_.NumComplaintsReceived(
                                    beacon_->manager.qual()) == beacon_->manager.qual().size() - 1)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(system_clock_));
  }

  if (timer_to_proceed_.HasExpired() || condition_to_proceed_)
  {
    qual_complaints_manager_.Finish(beacon_->manager.qual(), identity_.identifier());

    CheckQualComplaints();
    std::size_t const size = qual_complaints_manager_.ComplaintsSize();

    // Reset if complaints is over threshold as this breaks the initial assumption on the
    // number of Byzantine nodes
    if (size > beacon_->manager.polynomial_degree())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                     " DKG has failed: complaints size ", size, " greater than threshold.");
      SetTimeToProceed(State::RESET);
      beacon_dkg_state_failed_on_->set(static_cast<uint64_t>(state_machine_->state()));
      return State::RESET;
    }
    if (qual_complaints_manager_.FindComplaint(identity_.identifier()))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                     " is in qual complaints");
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
  FETCH_LOCK(mutex_);
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

  if (!condition_to_proceed_ && received_count == remaining_honest.size() - 1)
  {
    condition_to_proceed_ = true;
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " State: ", ToString(state_machine_->state()),
                   " Ready. Seconds to spare: ", state_deadline_ - GetTime(system_clock_));
  }

  if (timer_to_proceed_.HasExpired() || condition_to_proceed_)
  {
    // Process reconstruction shares. Reconstruction shares from non-qual members
    // or people in qual complaints should not be considered
    for (auto const &share : reconstruction_shares_received_)
    {
      MuddleAddress from = share.first;
      if (qual_complaints_manager_.FindComplaint(from) ||
          beacon_->manager.qual().find(from) == beacon_->manager.qual().end())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                       " received message from invalid sender. Discarding.");
        continue;
      }
      for (auto const &elem : share.second)
      {
        // Check person who's shares are being exposed is a member of qual
        if (beacon_->manager.qual().find(elem.first) != beacon_->manager.qual().end())
        {
          beacon_->manager.VerifyReconstructionShare(from, elem);
        }
      }
    }

    // Reset if reconstruction fails as this breaks the initial assumption on the
    // number of Byzantine nodes
    if (!beacon_->manager.RunReconstruction())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                     " DKG failed due to reconstruction failure. Resetting.");
      SetTimeToProceed(State::RESET);
      beacon_dkg_state_failed_on_->set(static_cast<uint64_t>(state_machine_->state()));
      return State::RESET;
    }

    SetTimeToProceed(State::COMPUTE_PUBLIC_SIGNATURE);
    return State::COMPUTE_PUBLIC_SIGNATURE;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_RECONSTRUCTION_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnComputePublicSignature()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::COMPUTE_PUBLIC_SIGNATURE));

  beacon_->manager.ComputePublicKeys();

  SetTimeToProceed(State::DRY_RUN_SIGNING);
  return State::DRY_RUN_SIGNING;
}

/**
 * Attempt to sign the seed to determine enough peers have generated the
 * group public signature. If successful, this should generate the first block entropy
 * structure.
 *
 * To do this, a block entropy struct specifying the qualified members, group signature etc. is
 * created, and nodes try and collect threshold signatures (personal) of that hash from members of
 * the qualified set.
 *
 */
// TODO(HUT): rename dry run to create first signature.
BeaconSetupService::State BeaconSetupService::OnDryRun()
{
  FETCH_LOCK(mutex_);

  // TODO(HUT): reset to qual here for the networking (?)

  // Only on first entry to this function, construct the structure
  if (state_machine_->previous_state() != State::DRY_RUN_SIGNING)
  {
    // Start to create the block entropy for this attempt
    beacon_->block_entropy                  = BlockEntropy{};
    beacon_->block_entropy.qualified        = beacon_->manager.qual();
    beacon_->block_entropy.group_public_key = beacon_->manager.group_public_key();
    beacon_->block_entropy.block_number     = beacon_->aeon.round_start;
    beacon_->block_entropy.HashSelf();

    assert(!beacon_->block_entropy.digest.empty());

    // Add own signature to the structure
    auto own_signature = certificate_->Sign(beacon_->block_entropy.digest);
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), " signs digest ",
                   beacon_->block_entropy.digest.ToHex());
    beacon_->block_entropy.confirmations.insert(
        {certificate_->identity().identifier(), own_signature});

    SendBroadcast(DKGEnvelope{FinalStateMessage{own_signature}});
  }

  // When the timer has expired, try to create the final structure
  if (timer_to_proceed_.HasExpired())
  {
    // Need at least DKG threshold number of signatures otherwise there will not be enough
    // parties who agree of the dkg output to compute threshold signatures
    auto desired_signatures_min = beacon_->manager.polynomial_degree() + 1;

    // For each sig, verify that it matches the hash
    for (auto const &address_and_sig : final_state_payload_)
    {
      if (crypto::Verifier::Verify(Identity(address_and_sig.first), beacon_->block_entropy.digest,
                                   address_and_sig.second))
      {
        beacon_->block_entropy.confirmations.insert(
            {address_and_sig.first, address_and_sig.second});
      }
      else
      {
        FETCH_LOG_INFO(
            LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
            "received invalid signature from node ",
            beacon_->manager.cabinet_index(address_and_sig.first),
            " when constructing block entropy. Other's signatures: ", final_state_payload_.size());
      }
    }

    // Current requirement: collect all signatures from qual.
    if (beacon_->block_entropy.confirmations.size() >= desired_signatures_min)
    {
      SetTimeToProceed(State::BEACON_READY);
      return State::BEACON_READY;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Failed to collect enough signatures. Collected: ",
                   beacon_->block_entropy.confirmations.size(),
                   " Desired: ", desired_signatures_min);
    SetTimeToProceed(State::RESET);
    beacon_dkg_state_failed_on_->set(static_cast<uint64_t>(state_machine_->state()));
    return State::RESET;
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::DRY_RUN_SIGNING;
}

BeaconSetupService::State BeaconSetupService::OnBeaconReady()
{
  FETCH_LOCK(mutex_);
  beacon_dkg_state_gauge_->set(static_cast<uint64_t>(State::BEACON_READY));
  beacon_dkg_successes_total_->add(1);
  beacon_dkg_miners_in_qual_->set(beacon_->manager.qual().size());

  FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                 " ******* New beacon generated! ******* Qual: ", beacon_->manager.qual().size(),
                 " of ", beacon_->aeon.members.size());

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
  std::string question = std::to_string(uint8_t(env.Message()->type())) +
                         ToString(state_machine_->state()) + std::to_string(failures_);
  rbc_->SetQuestion(ConstByteArray(question), serialiser.data());
}

/**
 * Randomly initialises coefficients of two polynomials, computes the coefficients and secret
 * shares and sends to cabinet members
 */
void BeaconSetupService::BroadcastShares()
{
  beacon_->manager.GenerateCoefficients();
  SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_SHARES),
                                                beacon_->manager.GetCoefficients()}});
  for (auto &cab_i : beacon_->aeon.members)
  {
    if (cab_i == identity_.identifier())
    {
      continue;
    }
    std::pair<MessageShare, MessageShare> shares{beacon_->manager.GetOwnShares(cab_i)};

    fetch::serializers::SizeCounter counter;
    counter << shares;

    fetch::serializers::MsgPackSerializer serializer;
    serializer.Reserve(counter.size());
    serializer << shares;
    endpoint_.Send(cab_i, SERVICE_DKG, CHANNEL_SECRET_KEY, serializer.data(),
                   MuddleEndpoint::OPTION_ENCRYPTED);
  }
  FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                  " broadcasts coefficients ");
}

/**
 * Broadcast the set nodes we are complaining against based on the secret shares and coefficients
 * sent to use. Also increments the number of complaints a given cabinet member has received with
 * our complaints
 */
void BeaconSetupService::BroadcastComplaints()
{
  std::unordered_set<MuddleAddress> complaints_local;

  // Add nodes who did not send both coefficients and shares to complaints
  for (auto const &member : beacon_->aeon.members)
  {
    if (member == identity_.identifier())
    {
      continue;
    }
    if (coefficients_received_.find(member) == coefficients_received_.end() ||
        shares_received_.find(member) == shares_received_.end())
    {
      complaints_local.insert(member);
    }
  }

  // Add nodes whos coefficients and shares failed verification to complaints
  auto verification_fail =
      beacon_->manager.ComputeComplaints(coefficients_received_ & shares_received_);
  complaints_local.insert(verification_fail.begin(), verification_fail.end());

  for (auto const &cab : complaints_local)
  {
    complaints_manager_.AddComplaintAgainst(cab);
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                  " broadcasts complaints size ", complaints_local.size());
  SendBroadcast(DKGEnvelope{ComplaintsMessage{complaints_local}});
}

/**
 * For a complaint by cabinet member c_i against self we broadcast the secret share
 * we sent to c_i to all cabinet members. This serves as a round of defense against
 * complaints where a member reveals the secret share they sent to c_i to everyone to
 * prove that it is consistent with the coefficients they originally broadcasted
 */
void BeaconSetupService::BroadcastComplaintAnswers()
{
  std::unordered_map<MuddleAddress, std::pair<MessageShare, MessageShare>> complaint_answer;
  for (auto const &reporter : complaints_manager_.ComplaintsAgainstSelf())
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                    " received complaints from ", beacon_->manager.cabinet_index(reporter));
    complaint_answer.insert({reporter, beacon_->manager.GetOwnShares(reporter)});
  }
  SendBroadcast(DKGEnvelope{
      SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_COMPLAINT_ANSWERS), complaint_answer}});
}

/**
 * Broadcast coefficients after computing own secret share
 */
void BeaconSetupService::BroadcastQualCoefficients()
{
  SendBroadcast(DKGEnvelope{CoefficientsMessage{static_cast<uint8_t>(State::WAIT_FOR_QUAL_SHARES),
                                                beacon_->manager.GetQualCoefficients()}});
}

/**
 * After constructing the qualified set (qual) and receiving new qual coefficients members
 * broadcast the secret shares s_ij, sprime_ij of all members in qual who sent qual coefficients
 * which failed verification
 */
void BeaconSetupService::BroadcastQualComplaints()
{
  // Qual complaints consist of all nodes from which we did not receive qual shares, or verification
  // of qual shares failed
  SendBroadcast(DKGEnvelope{
      SharesMessage{static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS),
                    beacon_->manager.ComputeQualComplaints(qual_coefficients_received_)}});
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
  SendBroadcast(DKGEnvelope{SharesMessage{
      static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES), complaint_shares}});
}

/**
 * Handler for DKGMessage that has passed through the reliable broadcast
 *
 * @param from Muddle address of sender
 * @param msg_ptr Pointer of DKGMessage
 */
void BeaconSetupService::OnDkgMessage(MuddleAddress const &              from,
                                      const std::shared_ptr<DKGMessage> &msg_ptr)
{
  FETCH_LOCK(mutex_);
  if (state_machine_->state() == State::IDLE || !BasicMsgCheck(from, msg_ptr))
  {
    return;
  }
  switch (msg_ptr->type())
  {
  case DKGMessage::MessageType::CONNECTIONS:
  {
    auto connections_ptr = std::dynamic_pointer_cast<ConnectionsMessage>(msg_ptr);

    ready_connections_.insert({from, connections_ptr->connections_});
    break;
  }
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
  case DKGMessage::MessageType::FINAL_STATE:
  {
    auto ptr = std::dynamic_pointer_cast<FinalStateMessage>(msg_ptr);
    if (ptr != nullptr)
    {
      if (beacon_->manager.qual().find(from) != beacon_->manager.qual().end() &&
          final_state_payload_.find(from) == final_state_payload_.end())
      {
        final_state_payload_.insert({from, ptr->payload_});
      }
    }
    break;
  }
  default:
    FETCH_LOG_ERROR(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
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
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                    " received complaint answer from ", from_index);
    OnComplaintAnswers(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAIT_FOR_QUAL_COMPLAINTS))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                    " received QUAL complaint from ", from_index);
    OnQualComplaints(shares, from_id);
  }
  else if (phase1 == static_cast<uint64_t>(State::WAIT_FOR_RECONSTRUCTION_SHARES))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
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

  std::pair<MessageShare, MessageShare> shares;
  serialiser >> shares;

  // Dispatch the event
  OnNewShares(packet.GetSender(), shares);
}

/**
 * Handler for RPC submit shares used for members to send individual pairs of
 * secret shares to other cabinet members
 *
 * @param from Muddle address of sender
 * @param shares Pair of secret shares
 */
void BeaconSetupService::OnNewShares(const MuddleAddress &                        from,
                                     std::pair<MessageShare, MessageShare> const &shares)
{
  FETCH_LOCK(mutex_);

  // This can occur if someone were to send you shares before you load the beacon
  if (!beacon_)
  {
    return;
  }

  // Check if sender is in cabinet
  bool in_cabinet{false};
  for (auto &member : beacon_->aeon.members)
  {
    if (member == from)
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
    beacon_->manager.AddShares(from, shares);
    FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                    " received shares from node  ", beacon_->manager.cabinet_index(from));
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
      beacon_->manager.AddCoefficients(from, msg.coefficients());
      FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                      " received coefficients from node  ", beacon_->manager.cabinet_index(from));
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
      beacon_->manager.AddQualCoefficients(from, msg.coefficients());
      FETCH_LOG_DEBUG(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                      " received qual coefficients from node  ",
                      beacon_->manager.cabinet_index(from));
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
  complaints_manager_.AddComplaintsFrom(from, msg.complaints(), beacon_->aeon.members);
}

/**
 * Handler for complaints answer message containing the pairs of secret shares the sender sent to
 * members that complained against the sender
 *
 * @param msg_ptr Pointer of SharesMessage containing the sender's shares
 * @param from_id Muddle address of sender
 */
void BeaconSetupService::OnComplaintAnswers(SharesMessage const &answer, MuddleAddress const &from)
{
  complaint_answers_manager_.AddComplaintAnswerFrom(from, answer.shares());
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
  qual_complaints_manager_.AddComplaintsFrom(from, shares_msg.shares());
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
void BeaconSetupService::CheckComplaintAnswers()
{
  auto answer_messages = complaint_answers_manager_.ComplaintAnswersReceived();
  for (auto const &sender_answers : answer_messages)
  {
    MuddleAddress                     from = sender_answers.first;
    std::unordered_set<MuddleAddress> answered_complaints;
    for (auto const &share : sender_answers.second)
    {
      // Check that the claimed submitter of the complaint actually did so
      if (complaints_manager_.FindComplaint(from, share.first))
      {
        answered_complaints.insert(share.first);
        if (!beacon_->manager.VerifyComplaintAnswer(from, share))
        {
          complaint_answers_manager_.AddComplaintAgainst(from);
        }
      }
    }

    // If not all complaints against from_id are answered then add a complaint against it
    if (answered_complaints.size() != complaints_manager_.ComplaintsCount(from))
    {
      complaint_answers_manager_.AddComplaintAgainst(from);
    }
  }
}

/**
 * Builds the set of qualified members of the cabinet.  Altogether, complaints consists of
  // 1. Nodes which received over t complaints
  // 2. Complaint answers which were false

 * @return True if self is in qual and qual is at least of size QualSize(), false
 otherwise
 */
bool BeaconSetupService::BuildQual()
{
  // Create set of muddle addresses
  std::set<MuddleAddress> cabinet;
  for (auto &member : beacon_->aeon.members)
  {
    cabinet.insert(member);
  }
  beacon_->manager.SetQual(complaint_answers_manager_.BuildQual(cabinet));
  std::set<MuddleAddress> qual = beacon_->manager.qual();
  if (qual.find(identity_.identifier()) == qual.end())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " build qual failed as not in qual. Qual size: ", qual.size());
    return false;
  }
  if (qual.size() < QualSize())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " build qual failed as size ", qual.size(), " less than required ", QualSize());
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
  for (const auto &complaint : qual_complaints_manager_.ComplaintsReceived(qual))
  {
    MuddleAddress sender = complaint.first;
    for (auto const &share : complaint.second)
    {
      // Check person who's shares are being exposed is not in QUAL then don't bother with checks
      if (qual.find(share.first) != qual.end())
      {
        qual_complaints_manager_.AddComplaintAgainst(
            beacon_->manager.VerifyQualComplaint(sender, share));
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
  if (!beacon_)
  {
    return false;
  }

  // Check if sender is in cabinet
  bool in_cabinet{false};
  for (auto &member : beacon_->aeon.members)
  {
    if (member == from)
    {
      in_cabinet = true;
    }
  }

  if (msg_ptr == nullptr)
  {
    return false;
  }
  if (!in_cabinet)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " received message from unknown sender");
    return false;
  }
  return true;
}

void BeaconSetupService::QueueSetup(const SharedAeonExecutionUnit &beacon)
{
  FETCH_LOCK(mutex_);
  assert(beacon != nullptr);

  aeon_exe_queue_.push_back(beacon);
}

void BeaconSetupService::Abort(uint64_t abort_below)
{
  FETCH_LOCK(mutex_);
  abort_below_ = abort_below;
}

void BeaconSetupService::SetBeaconReadyCallback(CallbackFunction callback)
{
  FETCH_LOCK(mutex_);
  callback_function_ = std::move(callback);
}

std::vector<std::weak_ptr<core::Runnable>> BeaconSetupService::GetWeakRunnables()
{
  return {state_machine_, rbc_->GetRunnable()};
}

double TimePerSlot(uint64_t cabinet_size)
{
  // Default
  uint64_t expected_dkg_time_s = 10 * (cabinet_size << 1u);

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
  if (cabinet_size < 51)
  {
    expected_dkg_time_s = 250;
  }
  if (cabinet_size < 30)
  {
    expected_dkg_time_s = 100;
  }
  if (cabinet_size < 10)
  {
    expected_dkg_time_s = 30;
  }
  if (cabinet_size < 8)
  {
    expected_dkg_time_s = 10;
  }

  // This assigns a time in seconds to each slot. Dividing by 100 is an arbitrary way to select the
  // time for a single slot from the total DKG time and we could equally just set the time per slot
  // directly.
  return static_cast<double>(expected_dkg_time_s) / 100.;
}

void BeaconSetupService::SetTimeBySlots(BeaconSetupService::State state, uint64_t &time_slots_total,
                                        uint64_t &time_slot_for_state)
{
  assert(time_slot_map_.find(state) != time_slot_map_.end());
  time_slot_for_state = time_slot_map_.at(state);

  while (state != BeaconSetupService::State::RESET)
  {
    time_slots_total += time_slot_map_.at(state);
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
  uint64_t current_time = GetTime(system_clock_);

  FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                 " determining time allowed to move on from state: \"", ToString(state), "\" at ",
                 current_time);
  condition_to_proceed_ = false;

  uint64_t cabinet_size  = beacon_->aeon.members.size();
  double   time_per_slot = TimePerSlot(cabinet_size);
  auto     expected_dkg_time_s =
      static_cast<uint64_t>(time_per_slot * static_cast<double>(time_slots_in_dkg_));

  FETCH_LOG_DEBUG(BeaconSetupService::LOGGING_NAME, "Note: Expect DKG time to be ",
                  expected_dkg_time_s, " s");

  // RESET state will delay DKG until the start point (or next start point)
  if (state == BeaconSetupService::State::RESET)
  {
    // Easy case where the start point is ahead in time
    // If not ahead in time, the DKG must have failed before. Algorithmically decide how long
    // to increase the allotted DKG time (scheme 2x)
    uint64_t next_start_point       = beacon_->aeon.start_reference_timepoint;
    uint64_t dkg_time               = expected_dkg_time_s;
    uint16_t failures               = 0;
    double   adjusted_time_per_slot = time_per_slot;

    while (next_start_point < current_time)
    {
      failures++;
      next_start_point += dkg_time;
      adjusted_time_per_slot += 0.5 * adjusted_time_per_slot;
      dkg_time =
          dkg_time + static_cast<uint64_t>(time_per_slot * static_cast<double>(time_slots_in_dkg_));
    }

    expected_dkg_timespan_ = dkg_time;
    reference_timepoint_   = next_start_point;
    time_per_slot_         = adjusted_time_per_slot;

    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " DKG round: ", beacon_->aeon.round_start, " failures so far: ", failures,
                   " allotted time: ", expected_dkg_timespan_, " base time: ", expected_dkg_time_s);

    beacon_dkg_time_allocated_->set(expected_dkg_timespan_);
    beacon_dkg_failures_required_to_complete_->set(failures);
    failures_ = failures;
  }

  // No timeout for these states
  if (state == BeaconSetupService::State::BEACON_READY || state == BeaconSetupService::State::IDLE)
  {
    return;
  }
  // Assign each state an equal amount of time for now
  uint64_t time_slots_total    = 0;
  uint64_t time_slot_for_state = 0;

  SetTimeBySlots(state, time_slots_total, time_slot_for_state);

  FETCH_LOG_DEBUG(
      LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), "Time per slot: ", time_per_slot_,
      ", time slots for state: ", time_slot_for_state, ", time slots total: ", time_slots_total);

  seconds_for_state_ = uint64_t(double(time_slot_for_state) * time_per_slot_);
  state_deadline_    = reference_timepoint_ + uint64_t(double(time_slots_total) * time_per_slot_);

  if (state_deadline_ < current_time)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(), " \n#### Deadline for ",
                   ToString(state), " has passed! This should not happen");
    timer_to_proceed_.Restart(0);
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", beacon_->manager.cabinet_index(),
                   " #### Proceeding to next state \"", ToString(state), "\", to last ",
                   state_deadline_ - current_time, " seconds (deadline: ", state_deadline_, ")");
    timer_to_proceed_.Restart(std::chrono::seconds{state_deadline_ - current_time});
  }
}

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
  case BeaconSetupService::State::COMPUTE_PUBLIC_SIGNATURE:
    text = "Compute the group public signature";
    break;
  case BeaconSetupService::State::DRY_RUN_SIGNING:
    text = "Dry run of signing a seed value";
    break;
  case BeaconSetupService::State::BEACON_READY:
    text = "Beacon ready";
  }

  return text;
}

}  // namespace beacon
}  // namespace fetch
