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

#include "beacon/aeon.hpp"
#include "beacon/beacon_service.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "muddle/muddle_interface.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"

#include "network/generics/milli_timer.hpp"

#include <chrono>
#include <iterator>

using fetch::generics::MilliTimer;

namespace fetch {
namespace beacon {

char const *ToString(BeaconService::State state);

BeaconService::BeaconService(MuddleInterface &               muddle,
                             ledger::ManifestCacheInterface &manifest_cache,
                             const CertificatePtr &certificate, SharedEventManager event_manager)
  : certificate_{certificate}
  , identity_{certificate->identity()}
  , endpoint_{muddle.GetEndpoint()}
  , state_machine_{std::make_shared<StateMachine>("BeaconService", State::WAIT_FOR_SETUP_COMPLETION,
                                                  ToString)}
  , rpc_client_{"BeaconService", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , event_manager_{std::move(event_manager)}
  , cabinet_creator_{muddle, identity_, manifest_cache, certificate_}
  , beacon_protocol_{*this}
  , beacon_entropy_generated_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_entropy_generated_total", "The total number of times entropy has been generated")}
  , beacon_entropy_future_signature_seen_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_entropy_future_signature_seen_total",
        "The total number of times entropy has been generated")}
  , beacon_entropy_forced_to_time_out_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_entropy_forced_to_time_out_total",
        "The total number of times entropy failed and timed out")}
  , beacon_entropy_last_requested_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_entropy_last_requested", "The last entropy value requested from the beacon")}
  , beacon_entropy_last_generated_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_entropy_last_generated", "The last entropy value able to be generated")}
  , beacon_entropy_current_round_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_entropy_current_round", "The current round attempting to generate for.")}
{

  // Attaching beacon ready callback handler
  cabinet_creator_.SetBeaconReadyCallback([this](SharedAeonExecutionUnit beacon) {
    FETCH_LOCK(mutex_);
    aeon_exe_queue_.push_back(beacon);
  });

  // Attaching the protocol
  rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_DKG, CHANNEL_RPC);
  rpc_server_->Add(RPC_BEACON, &beacon_protocol_);

  state_machine_->RegisterHandler(State::WAIT_FOR_SETUP_COMPLETION, this,
                                  &BeaconService::OnWaitForSetupCompletionState);
  state_machine_->RegisterHandler(State::PREPARE_ENTROPY_GENERATION, this,
                                  &BeaconService::OnPrepareEntropyGeneration);
  state_machine_->RegisterHandler(State::COLLECT_SIGNATURES, this,
                                  &BeaconService::OnCollectSignaturesState);
  state_machine_->RegisterHandler(State::VERIFY_SIGNATURES, this,
                                  &BeaconService::OnVerifySignaturesState);
  state_machine_->RegisterHandler(State::COMPLETE, this, &BeaconService::OnCompleteState);

  state_machine_->OnStateChange([this](State current, State previous) {
    FETCH_UNUSED(this);
    FETCH_UNUSED(current);
    FETCH_UNUSED(previous);
    FETCH_LOG_DEBUG(LOGGING_NAME, "Current state: ", ToString(current),
                    " (previous: ", ToString(previous), ")");
  });
}

BeaconService::Status BeaconService::GenerateEntropy(uint64_t block_number, BlockEntropy &entropy)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Requesting entropy for block number: ", block_number);
  beacon_entropy_last_requested_->set(block_number);

  FETCH_LOCK(mutex_);

  if (completed_block_entropy_.find(block_number) != completed_block_entropy_.end())
  {
    entropy = *completed_block_entropy_[block_number];
    return Status::OK;
  }

  return Status::FAILED;
}

void BeaconService::AbortCabinet(uint64_t round_start)
{
  cabinet_creator_.Abort(round_start);
}

void BeaconService::StartNewCabinet(CabinetMemberList members, uint32_t threshold,
                                    uint64_t round_start, uint64_t round_end, uint64_t start_time,
                                    BlockEntropy const &prev_entropy)
{
  auto diff_time = int64_t(static_cast<uint64_t>(std::time(nullptr))) - int64_t(start_time);
  FETCH_LOG_INFO(LOGGING_NAME, "Starting new cabinet from ", round_start, " to ", round_end,
                 "at time: ", start_time, " (diff): ", diff_time);

  // Check threshold meets the requirements for the RBC
  uint32_t rbc_threshold{0};
  if (members.size() % 3 == 0)
  {
    rbc_threshold = static_cast<uint32_t>(members.size() / 3 - 1);
  }
  else
  {
    rbc_threshold = static_cast<uint32_t>(members.size() / 3);
  }
  if (threshold < rbc_threshold)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Threshold is below RBC threshold. Reset to rbc threshold");
    threshold = rbc_threshold;
  }

  FETCH_LOCK(mutex_);

  SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

  beacon->manager.SetCertificate(certificate_);
  beacon->manager.NewCabinet(members, threshold);

  // Setting the aeon details
  beacon->aeon.round_start               = round_start;
  beacon->aeon.round_end                 = round_end;
  beacon->aeon.members                   = std::move(members);
  beacon->aeon.start_reference_timepoint = start_time;
  beacon->aeon.block_entropy_previous    = prev_entropy;

  cabinet_creator_.QueueSetup(beacon);
}

BeaconService::State BeaconService::OnWaitForSetupCompletionState()
{
  FETCH_LOCK(mutex_);

  // Checking whether the next committee is ready
  // to produce random numbers.
  if (!aeon_exe_queue_.empty())
  {
    active_exe_unit_ = aeon_exe_queue_.front();
    aeon_exe_queue_.pop_front();

    // Set the previous block entropy appropriately
    block_entropy_previous_ =
        std::make_shared<BlockEntropy>(active_exe_unit_->aeon.block_entropy_previous);
    block_entropy_being_created_ = std::make_shared<BlockEntropy>(active_exe_unit_->block_entropy);

    // TODO(HUT): re-enable this check after fixing the dealer test
    /* assert(block_entropy_being_created_->IsAeonBeginning()); */

    return State::PREPARE_ENTROPY_GENERATION;
  }

  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::WAIT_FOR_SETUP_COMPLETION;
}

BeaconService::State BeaconService::OnPrepareEntropyGeneration()
{
  FETCH_LOCK(mutex_);

  // Set the manager up to generate the signature
  active_exe_unit_->manager.SetMessage(block_entropy_previous_->EntropyAsSHA256());
  active_exe_unit_->member_share = active_exe_unit_->manager.Sign();

  // Ready to broadcast signatures
  return State::COLLECT_SIGNATURES;
}

BeaconService::State BeaconService::OnCollectSignaturesState()
{

  FETCH_LOCK(mutex_);

  uint64_t const index = block_entropy_being_created_->block_number;
  beacon_entropy_current_round_->set(index);

  // On first entry to function, populate with our info (will go between collect and verify)
  if (state_machine_->previous_state() == State::PREPARE_ENTROPY_GENERATION)
  {
    SignatureInformation this_round;
    this_round.round                                        = index;
    this_round.threshold_signatures[identity_.identifier()] = active_exe_unit_->member_share;
    signatures_being_built_[index]                          = this_round;

    // TODO(HUT): clean historically old sigs + entropy here
  }

  // Attempt to get signatures from a peer we do not have the signature of
  auto        missing_signatures_from = active_exe_unit_->manager.qual();
  auto const &signatures_struct       = signatures_being_built_[index];

  for (auto it = missing_signatures_from.begin(); it != missing_signatures_from.end();)
  {
    // If we have already seen, remove
    if (signatures_struct.threshold_signatures.find(*it) !=
        signatures_struct.threshold_signatures.end())
    {
      it = missing_signatures_from.erase(it);
    }
    else
    {
      ++it;
    }
  }

  if (missing_signatures_from.empty())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Signatures from all qual are already fulfilled");
    return State::VERIFY_SIGNATURES;
  }

  // semi randomly select a qual member we haven't got the signature information from to query
  std::size_t random_member_index = random_number_++ % missing_signatures_from.size();
  auto        it = std::next(missing_signatures_from.begin(), long(random_member_index));

  qual_promise_identity_ = Identity(*it);
  sig_share_promise_ =
      rpc_client_.CallSpecificAddress(qual_promise_identity_.identifier(), RPC_BEACON,
                                      BeaconServiceProtocol::GET_SIGNATURE_SHARES, index);

  // Note: this delay is effectively how long we wait for the network event to resolve
  state_machine_->Delay(std::chrono::milliseconds(50));

  return State::VERIFY_SIGNATURES;
}

BeaconService::State BeaconService::OnVerifySignaturesState()
{
  SignatureInformation ret;

  try
  {
    // Attempt to resolve the promise and add it
    if (!sig_share_promise_->IsSuccessful() || !sig_share_promise_->As<SignatureInformation>(ret))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to resolve RPC promise from ",
                     qual_promise_identity_.identifier().ToBase64());
      return State::COLLECT_SIGNATURES;
    }
  }
  catch (...)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Promise timed out and threw! This should not happen.");
  }

  // Note: don't lock until the promise has resolved (above)! Otherwise the system can deadlock
  // due to everyone trying to lock and resolve each others' signatures
  {
    FETCH_LOCK(mutex_);

    uint64_t const index = block_entropy_being_created_->block_number;

    if (ret.threshold_signatures.empty())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Peer wasn't ready when asking for signatures: ",
                      qual_promise_identity_.identifier().ToBase64());
      state_machine_->Delay(std::chrono::milliseconds(100));

      return State::COLLECT_SIGNATURES;
    }

    if (ret.round != index)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Peer returned the wrong round when asked for signatures. Peer: ",
                     qual_promise_identity_.identifier().ToBase64(), " returned: ", ret.round,
                     " expected: ", index);
      return State::COLLECT_SIGNATURES;
    }

    // Success - Add relevant info
    auto &signatures_struct = signatures_being_built_[index];
    auto &all_sigs_map      = signatures_struct.threshold_signatures;

    for (auto const &address_sig_pair : ret.threshold_signatures)
    {
      all_sigs_map[address_sig_pair.first] = address_sig_pair.second;
      // Let the manager know
      AddSignature(address_sig_pair.second);

      // If we have collected enough signatures already then break
      if (active_exe_unit_->manager.can_verify())
      {
        break;
      }
    }

    FETCH_LOG_INFO(LOGGING_NAME, "After adding, we have ", all_sigs_map.size(),
                   " signatures. Round: ", index);
  }  // Mutex unlocks here since verification can take some time

  MilliTimer const timer{"Verify threshold signature", 100};

  // TODO(HUT): possibility for infinite loop here I suspect.
  if (active_exe_unit_->manager.can_verify() && active_exe_unit_->manager.Verify())
  {
    return State::COMPLETE;
  }

  return State::COLLECT_SIGNATURES;
}

/**
 * Entropy has been successfully generated. Set up the next entropy gen or rotate committee
 */
BeaconService::State BeaconService::OnCompleteState()
{
  FETCH_LOCK(mutex_);

  uint64_t const index = block_entropy_being_created_->block_number;
  beacon_entropy_last_generated_->set(index);
  beacon_entropy_generated_total_->add(1);

  // Populate the block entropy structure appropriately TODO(HUT): this.
  // block_entropy_being_created_->group_signature = active_exe_unit_->manager.GroupSignature();

  // Save it for querying
  completed_block_entropy_[index] = block_entropy_being_created_;

  // If there is still entropy left to generate, set up and go around the loop
  if (block_entropy_being_created_->block_number < active_exe_unit_->aeon.round_end)
  {
    block_entropy_previous_                    = std::move(block_entropy_being_created_);
    block_entropy_being_created_               = std::make_shared<BlockEntropy>();
    block_entropy_being_created_->block_number = block_entropy_previous_->block_number + 1;

    return State::PREPARE_ENTROPY_GENERATION;
  }

  EventCommitteeCompletedWork event;
  event_manager_->Dispatch(event);

  return State::WAIT_FOR_SETUP_COMPLETION;
}

bool BeaconService::AddSignature(SignatureShare share)
{
  assert(active_exe_unit_ != nullptr);
  auto ret = active_exe_unit_->manager.AddSignaturePart(share.identity, share.signature);

  // Checking that the signature is valid
  if (ret == BeaconManager::AddResult::INVALID_SIGNATURE)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Signature invalid.");

    EventInvalidSignature event;
    // TODO(tfr): Received invalid signature - fill event details
    event_manager_->Dispatch(event);

    return false;
  }
  if (ret == BeaconManager::AddResult::NOT_MEMBER)
  {  // And that it was sent by a member of the cabinet
    FETCH_LOG_ERROR(LOGGING_NAME, "Signature from non-member.");

    EventSignatureFromNonMember event;
    // TODO(tfr): Received signature from non-member - deal with it.
    event_manager_->Dispatch(event);

    return false;
  }

  if (ret == BeaconManager::AddResult::SIGNATURE_ALREADY_ADDED)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Accidental duplicate signature added!");
  }

  return true;
}

std::vector<std::weak_ptr<core::Runnable>> BeaconService::GetWeakRunnables()
{
  std::vector<std::weak_ptr<core::Runnable>> ret = {state_machine_};

  auto setup_runnables = cabinet_creator_.GetWeakRunnables();

  for (auto const &i : setup_runnables)
  {
    ret.push_back(i);
  }

  return ret;
}

/**
 * Peers can call this function (RPC endpoint) to get threshold signatures that
 * this peer has collected
 */
BeaconService::SignatureInformation BeaconService::GetSignatureShares(uint64_t round)
{
  FETCH_LOCK(mutex_);

  // If this is too far in the future or the past return empty struct
  if (signatures_being_built_.find(round) == signatures_being_built_.end())
  {
    return {};
  }

  return signatures_being_built_.at(round);
}

char const *ToString(BeaconService::State state)
{
  char const *text = "unknown";

  switch (state)
  {
  case BeaconService::State::WAIT_FOR_SETUP_COMPLETION:
    text = "Waiting for setup completion";
    break;
  case BeaconService::State::PREPARE_ENTROPY_GENERATION:
    text = "Preparing entropy generation";
    break;
  case BeaconService::State::COLLECT_SIGNATURES:
    text = "Collecting signatures";
    break;
  case BeaconService::State::VERIFY_SIGNATURES:
    text = "Verifying signatures";
    break;
  case BeaconService::State::COMPLETE:
    text = "Completion state";
    break;
  case BeaconService::State::COMITEE_ROTATION:
    text = "Decide on committee rotation";
    break;
  case BeaconService::State::OBSERVE_ENTROPY_GENERATION:
    text = "Observe entropy generation";
    break;
  case BeaconService::State::WAIT_FOR_PUBLIC_KEYS:
    text = "Waiting for public keys";
    break;
  }

  return text;
}

}  // namespace beacon
}  // namespace fetch
