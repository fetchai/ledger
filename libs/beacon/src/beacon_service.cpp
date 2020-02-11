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

#include "beacon/aeon.hpp"
#include "beacon/beacon_service.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/containers/trim_to_size.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "muddle/muddle_interface.hpp"
#include "network/generics/milli_timer.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/utils/timer.hpp"
#include "telemetry/utils/to_seconds.hpp"

#include <chrono>
#include <iterator>
#include <random>

using namespace std::chrono_literals;

using fetch::core::TrimToSize;
using fetch::generics::MilliTimer;

namespace fetch {
namespace beacon {

namespace {

template <typename T>
T ChooseRandomlyFrom(T &container, std::size_t items)
{
  auto copy = container;

  thread_local std::random_device rd;
  thread_local std::mt19937       rng(rd());
  using Distribution = std::uniform_int_distribution<std::size_t>;

  while (!copy.empty() && copy.size() > items)
  {
    Distribution table_dist{0, copy.size() - 1};

    std::size_t const element = table_dist(rng);

    // advance the iterator to the correct offset
    auto it = copy.cbegin();
    std::advance(it, static_cast<std::ptrdiff_t>(element));

    // delete
    copy.erase(it);
  }

  return copy;
}

}  // namespace

char const *ToString(BeaconService::State state);

// Since it is annoying to serialize the state, an enum,
// we create a wrapper for serializing the beacon service
struct BeaconServiceSerializeWrapper
{
  BeaconService &beacon_service;
  uint16_t       current_state{0};
};

BeaconService::BeaconService(MuddleInterface &muddle, const CertificatePtr &certificate,
                             BeaconSetupService &beacon_setup, SharedEventManager event_manager,
                             bool load_and_reload_on_crash)
  : certificate_{certificate}
  , identity_{certificate->identity()}
  , muddle_{muddle}
  , endpoint_{muddle_.GetEndpoint()}
  , state_machine_{std::make_shared<StateMachine>("BeaconService", State::RELOAD_ON_STARTUP,
                                                  ToString)}
  , load_and_reload_on_crash_{load_and_reload_on_crash}
  , rpc_client_{"BeaconService", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , event_manager_{std::move(event_manager)}
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
  , beacon_state_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_state_gauge", "State the beacon is in as integer")}
  , beacon_most_recent_round_seen_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_most_recent_round_seen", "Most recent round the beacon has seen")}
  , beacon_collect_time_{telemetry::Registry::Instance().CreateHistogram(
        {0.000001, 0.00001, 0.0001, 0.001, 0.01, 0.1}, "beacon_collect_time",
        "Time taken to collect signatures")}
  , beacon_verify_time_{telemetry::Registry::Instance().CreateHistogram(
        {0.000001, 0.00001, 0.0001, 0.001, 0.01, 0.1}, "beacon_verify_time",
        "Time taken to verify signatures")}
{
  // Attaching beacon ready callback handler
  beacon_setup.SetBeaconReadyCallback([this](SharedAeonExecutionUnit beacon) {
    FETCH_LOCK(mutex_);
    aeon_exe_queue_.push_back(beacon);
  });

  // Attaching the protocol
  rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_DKG, CHANNEL_RPC);
  rpc_server_->Add(RPC_BEACON, &beacon_protocol_);

  // clang-format off
  state_machine_->RegisterHandler(State::RELOAD_ON_STARTUP, this, &BeaconService::OnReloadOnStartup);
  state_machine_->RegisterHandler(State::WAIT_FOR_SETUP_COMPLETION, this, &BeaconService::OnWaitForSetupCompletionState);
  state_machine_->RegisterHandler(State::PREPARE_ENTROPY_GENERATION, this, &BeaconService::OnPrepareEntropyGeneration);
  state_machine_->RegisterHandler(State::COLLECT_SIGNATURES, this, &BeaconService::OnCollectSignaturesState);
  state_machine_->RegisterHandler(State::VERIFY_SIGNATURES, this, &BeaconService::OnVerifySignaturesState);
  state_machine_->RegisterHandler(State::COMPLETE, this, &BeaconService::OnCompleteState);
  // clang-format on

  state_machine_->OnStateChange([this](State current, State previous) {
    FETCH_UNUSED(this);
    FETCH_UNUSED(current);
    FETCH_UNUSED(previous);
    FETCH_LOG_DEBUG(LOGGING_NAME, "Current state: ", ToString(current),
                    " (previous: ", ToString(previous), ")");
  });
}

// Helper function to create an inexpensive resource ID
template <typename T>
storage::ResourceID CreateRID(T from)
{
  byte_array::ByteArray memory_area(32);
  memory_area.Resize(32);

  memcpy(memory_area.pointer(), reinterpret_cast<char *>(&from), sizeof(T));

  return storage::ResourceID{memory_area};
}

/**
 * Save the exact state of the beacon service for later recovery. Note that this must be used
 * carefully, for example when recovering certain timers might not be set and it could jump
 * into a state that expects it to be.
 *
 */
void BeaconService::SaveState()
{
  if (!load_and_reload_on_crash_)
  {
    return;
  }

  MilliTimer const timer{"SaveState ", 300};

  try
  {
    if (!signatures_being_built_.empty())
    {
      // The signatures are stored in a separate structure so they can be written in incrementally.
      // First, clean possible old signatures (assume they are always contiguous)
      uint64_t lowest_relevant_sig_index  = signatures_being_built_.begin()->first;
      uint64_t highest_relevant_sig_index = signatures_being_built_.rbegin()->first;

      // Note the - 1 here to continually remove the one before
      while (lowest_relevant_sig_index != 0 &&
             saved_state_all_sigs_.Has(CreateRID(lowest_relevant_sig_index - 1)))
      {
        saved_state_all_sigs_.Erase(CreateRID(lowest_relevant_sig_index - 1));
        lowest_relevant_sig_index--;
      }

      // Now add signatures which are new
      while (!saved_state_all_sigs_.Has(CreateRID(highest_relevant_sig_index)) &&
             signatures_being_built_.find(highest_relevant_sig_index) !=
                 signatures_being_built_.end())
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Highest relevant: ", highest_relevant_sig_index,
                        " size: ", signatures_being_built_.size());
        saved_state_all_sigs_.Set(CreateRID(highest_relevant_sig_index),
                                  signatures_being_built_.at(highest_relevant_sig_index));
        highest_relevant_sig_index--;
      }
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to save beacon service sigs to file: ", ex.what());
  }

  try
  {
    saved_state_.Set(
        BeaconServiceSerializeWrapper{*this, static_cast<uint16_t>(state_machine_->state())});
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to save beacon service to file: ", ex.what());
  }
}

void BeaconService::ReloadState(State &next_state)
{
  old_state_.Load("beacon_state.db", "beacon_state.index.db");  // Legacy/depreciated
  saved_state_all_sigs_.Load("beacon_state_sigs_v2.db", "beacon_state_sigs_v2.index.db");
  saved_state_.Load("beacon_state_v2.db");

  if (!load_and_reload_on_crash_)
  {
    return;
  }

  bool loaded_state = false;

  try
  {
    // Load all signatures from the file
    for (auto const &siginfo : saved_state_all_sigs_)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Adding sigs for: ", siginfo.round);
      signatures_being_built_[siginfo.round] = siginfo;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Loaded signatures. Attemping loading of the rest of the class.");

    BeaconServiceSerializeWrapper wrapper{*this, 0};

    saved_state_.Get(wrapper);

    next_state = static_cast<State>(wrapper.current_state);

    FETCH_LOG_INFO(LOGGING_NAME, "After re-load state is: ", wrapper.current_state);

    // Note, since certificates are not serialized, we must set the beacon managers in the aeon
    // to have the correct one
    if (active_exe_unit_)
    {
      active_exe_unit_->manager.SetCertificate(certificate_);

      // For performance, make connections to all of qual
      for (auto const &address_in_qual : active_exe_unit_->manager.qual())
      {
        muddle_.ConnectTo(address_in_qual);
      }
    }

    for (auto const &i : aeon_exe_queue_)
    {
      if (i)
      {
        i->manager.SetCertificate(certificate_);
      }
    }

    loaded_state = true;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to load beacon service from storage: ", ex.what());
  }

  if (!loaded_state)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Failed to load state. Attempting to load legacy file.");

    SharedAeonExecutionUnit ret = std::make_shared<AeonExecutionUnit>();

    FETCH_LOG_INFO(LOGGING_NAME, "Reloading... Size: ", old_state_.size());

    if (old_state_.Get(storage::ResourceAddress("HEAD"), *ret))
    {
      FETCH_LOG_INFO(LOGGING_NAME,
                     "Found aeon keys during beacon construction, recovering. Valid from: ",
                     ret->aeon.round_start, " to ", ret->aeon.round_end);

      for (auto const &address_in_qual : ret->manager.qual())
      {
        muddle_.ConnectTo(address_in_qual);
      }

      ret->manager.SetCertificate(certificate_);

      aeon_exe_queue_.push_back(ret);
    }
  }
}

/**
 * This function will be called once after construction and will attempt to recover the beacon
 * service exactly to the last saved state.
 */
BeaconService::State BeaconService::OnReloadOnStartup()
{
  // Default starting state, will be overwritten on successful load
  State state_after_reload = State::WAIT_FOR_SETUP_COMPLETION;

  ReloadState(state_after_reload);

  FETCH_LOG_INFO(LOGGING_NAME, "After reloading state, we have ", completed_block_entropy_.size(),
                 " completed block entropy");

  return state_after_reload;
}

BeaconService::Status BeaconService::GenerateEntropy(uint64_t block_number, BlockEntropy &entropy)
{
  FETCH_LOG_TRACE(LOGGING_NAME, "Requesting entropy for block number: ", block_number);
  beacon_entropy_last_requested_->set(block_number);

  FETCH_LOCK(mutex_);

  if (completed_block_entropy_.find(block_number) != completed_block_entropy_.end())
  {
    entropy = *completed_block_entropy_[block_number];
    return Status::OK;
  }

  return Status::FAILED;
}

void BeaconService::MostRecentSeen(uint64_t round)
{
  FETCH_LOCK(mutex_);
  most_recent_round_seen_ = round;
  beacon_most_recent_round_seen_->set(most_recent_round_seen_);
}

// Determine whether signatures already exist (used rather than entropy
// since it is persisted across reboots)
bool SigsAlreadyExist(BeaconService::SharedAeonExecutionUnit const &exe_unit,
                      BeaconService::SignaturesBeingBuilt const &sigs, char const *LOGGING_NAME)
{
  if (sigs.empty())
  {
    return false;
  }

  // If the aeon is correct, its beginning should be higher than all previously generated
  // signatures. Check last signature against beginning of aeon
  BeaconService::SignatureInformation const &last_signature = sigs.crbegin()->second;

  if (exe_unit->aeon.round_start > last_signature.round)
  {
    return false;
  }

  FETCH_LOG_WARN(
      LOGGING_NAME,
      "Found an aeon in the aeon queue that appears to be old or a duplicate! It starts: ",
      exe_unit->aeon.round_start, " but we have a signature for round: ", last_signature.round);

  return true;
}

BeaconService::State BeaconService::OnWaitForSetupCompletionState()
{
  beacon_state_gauge_->set(static_cast<uint64_t>(state_machine_->state()));
  FETCH_LOCK(mutex_);

  active_exe_unit_.reset();

  // Checking whether the next cabinet is ready
  // to produce random numbers.
  if (!aeon_exe_queue_.empty())
  {
    active_exe_unit_ = aeon_exe_queue_.front();
    aeon_exe_queue_.pop_front();

    // Guard against erroneous DKG creation if the network were to reset and resync
    if (SigsAlreadyExist(active_exe_unit_, signatures_being_built_, LOGGING_NAME))
    {
      active_exe_unit_.reset();
    }
    else
    {
      // Set the previous block entropy appropriately
      block_entropy_previous_ =
          std::make_shared<BlockEntropy>(active_exe_unit_->aeon.block_entropy_previous);
      block_entropy_being_created_ =
          std::make_shared<BlockEntropy>(active_exe_unit_->block_entropy);

      // TODO(HUT): re-enable this check after fixing the dealer test
      /* assert(block_entropy_being_created_->IsAeonBeginning()); */

      return State::PREPARE_ENTROPY_GENERATION;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::WAIT_FOR_SETUP_COMPLETION;
}

// Attempt to determine whether the node is attempting to generate entropy that is far in the past
// (possible on start up when syncing)
bool BeaconService::OutOfSync()
{
  if (most_recent_round_seen_ > active_exe_unit_->aeon.round_end)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "most recent seen exceeds current exe bounds: most recent: ",
                   most_recent_round_seen_, " current: ", active_exe_unit_->aeon.round_end);
  }

  return most_recent_round_seen_ > active_exe_unit_->aeon.round_end;
}

BeaconService::State BeaconService::OnPrepareEntropyGeneration()
{
  beacon_state_gauge_->set(static_cast<uint64_t>(state_machine_->state()));
  FETCH_LOCK(mutex_);

  uint64_t const index = block_entropy_being_created_->block_number;

  // Save state to disk in case of crash - it should return to this
  // state in the state machine with all relevant items setup
  if ((index % SAVE_PERIODICITY) == 0 ||
      state_machine_->previous_state() == State::WAIT_FOR_SETUP_COMPLETION)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Periodically saving the entropy information. Index: ", index);
    SaveState();
  }

  if (OutOfSync())
  {
    return State::WAIT_FOR_SETUP_COMPLETION;
  }

  // Set the manager up to generate the signature
  active_exe_unit_->manager.SetMessage(block_entropy_previous_->EntropyAsSHA256());
  active_exe_unit_->member_share = active_exe_unit_->manager.Sign();

  // Ready to broadcast signatures
  return State::COLLECT_SIGNATURES;
}

BeaconService::State BeaconService::OnCollectSignaturesState()
{
  beacon_state_gauge_->set(static_cast<uint64_t>(state_machine_->state()));

  started_request_for_sigs_ = Clock::now();

  FETCH_LOCK(mutex_);

  if (OutOfSync())
  {
    return State::WAIT_FOR_SETUP_COMPLETION;
  }

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

  // Don't proceed from this state if it is ahead of the entropy we are trying to generate,
  // or we have no peers
  if (index > (most_recent_round_seen_ + entropy_lead_blocks_) ||
      endpoint_.GetDirectlyConnectedPeers().empty())
  {
    state_machine_->Delay(std::chrono::milliseconds(5));
    return State::COLLECT_SIGNATURES;
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
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Signatures from all qual are already fulfilled. Re-querying a random node");
    missing_signatures_from = ChooseRandomlyFrom(active_exe_unit_->manager.qual(), 1);
  }

  // semi randomly select a qual member we haven't got the signature information from to query
  std::size_t random_member_index = random_number_++ % missing_signatures_from.size();
  auto        it = std::next(missing_signatures_from.begin(), long(random_member_index));

  FETCH_LOG_DEBUG(LOGGING_NAME, "Get Signature shares... (index: ", index, ")");

  qual_promise_identity_ = Identity(*it);
  sig_share_promise_ =
      rpc_client_.CallSpecificAddress(qual_promise_identity_.identifier(), RPC_BEACON,
                                      BeaconServiceProtocol::GET_SIGNATURE_SHARES, index);

  // Timer to wait maximally for network events
  timer_to_proceed_.Restart(std::chrono::milliseconds{200});

  return State::VERIFY_SIGNATURES;
}

BeaconService::State BeaconService::OnVerifySignaturesState()
{
  beacon_state_gauge_->set(static_cast<uint64_t>(state_machine_->state()));
  SignatureInformation ret{};
  uint64_t             index = 0;

  {
    FETCH_LOCK(mutex_);
    index = block_entropy_being_created_->block_number;
  }

  if (OutOfSync())
  {
    return State::WAIT_FOR_SETUP_COMPLETION;
  }

  // Block for up to half a second waiting for the promise to resolve
  if (!timer_to_proceed_.HasExpired() && !sig_share_promise_->IsSuccessful())
  {
    state_machine_->Delay(std::chrono::milliseconds(50));
    return State::VERIFY_SIGNATURES;
  }

  try
  {
    // Attempt to resolve the promise and add it
    if (!sig_share_promise_->IsSuccessful() || !sig_share_promise_->GetResult(ret))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to resolve RPC promise from ",
                     qual_promise_identity_.identifier().ToBase64(),
                     " when generating entropy for block: ", index,
                     " connections: ", endpoint_.GetDirectlyConnectedPeers().size());

      state_machine_->Delay(std::chrono::milliseconds(100));
      return State::COLLECT_SIGNATURES;
    }
  }
  catch (...)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Promise timed out and threw! This should not happen.");
  }

  // Now collected
  beacon_collect_time_->Add(
      telemetry::details::ToSeconds(Clock::now() - started_request_for_sigs_));

  MilliTimer const               timer{"Verify collective threshold signature", 100};
  telemetry::FunctionTimer const timer1{*beacon_verify_time_};

  // Note: don't lock until the promise has resolved (above)! Otherwise the system can deadlock
  // due to everyone trying to lock and resolve each others' signatures
  {
    FETCH_LOCK(mutex_);

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

    FETCH_LOG_DEBUG(LOGGING_NAME, "After adding, we have ", all_sigs_map.size(),
                    " signatures. Round: ", index);
  }  // Mutex unlocks here since verification can take some time

  MilliTimer const timer2{"Verify threshold signature", 100};

  // TODO(HUT): possibility for infinite loop here I suspect.
  if (active_exe_unit_->manager.can_verify() && active_exe_unit_->manager.Verify())
  {
    return State::COMPLETE;
  }

  return State::COLLECT_SIGNATURES;
}

/**
 * Entropy has been successfully generated. Set up the next entropy gen or rotate cabinet
 */
BeaconService::State BeaconService::OnCompleteState()
{
  beacon_state_gauge_->set(static_cast<uint64_t>(state_machine_->state()));
  FETCH_LOCK(mutex_);

  if (OutOfSync())
  {
    return State::WAIT_FOR_SETUP_COMPLETION;
  }

  uint64_t const index = block_entropy_being_created_->block_number;
  beacon_entropy_last_generated_->set(index);
  beacon_entropy_generated_total_->add(1);

  // Populate the block entropy structure appropriately
  block_entropy_being_created_->group_signature =
      active_exe_unit_->manager.GroupSignature().getStr();

  // Check when in debug mode that the block entropy signing has gone correctly
  assert(dkg::BeaconManager::Verify(block_entropy_being_created_->group_public_key,
                                    block_entropy_previous_->EntropyAsSHA256(),
                                    block_entropy_being_created_->group_signature));

  // Trim maps of unnecessary info
  auto const max_cache_size =
      ((active_exe_unit_->aeon.round_end - active_exe_unit_->aeon.round_start) + 1) * 3;

  TrimToSize(completed_block_entropy_, max_cache_size);
  TrimToSize(signatures_being_built_, max_cache_size);

  // Save it for querying
  completed_block_entropy_[index] = block_entropy_being_created_;

  // If there is still entropy left to generate, set up and go around the loop
  if (block_entropy_being_created_->block_number < active_exe_unit_->aeon.round_end)
  {
    block_entropy_previous_      = std::move(block_entropy_being_created_);
    block_entropy_being_created_ = std::make_shared<BlockEntropy>();
    block_entropy_being_created_->SelectCopy(*block_entropy_previous_);
    block_entropy_being_created_->block_number = block_entropy_previous_->block_number + 1;

    return State::PREPARE_ENTROPY_GENERATION;
  }

  EventCabinetCompletedWork event;
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
    FETCH_LOG_ERROR(LOGGING_NAME, "Signature from non-member! Identity: ",
                    share.identity.identifier().ToBase64());

    for (auto const &i : active_exe_unit_->manager.qual())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Note: qual is: ", i.ToBase64());
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Note: we are: ", identity_.identifier().ToBase64());

    EventSignatureFromNonMember event;
    // TODO(tfr): Received signature from non-member - deal with it.
    event_manager_->Dispatch(event);

    return false;
  }

  if (ret == BeaconManager::AddResult::SIGNATURE_ALREADY_ADDED)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Accidental duplicate signature added!");
  }

  return true;
}

std::weak_ptr<core::Runnable> BeaconService::GetWeakRunnable()
{
  std::weak_ptr<core::Runnable> ret = {state_machine_};

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
  case BeaconService::State::RELOAD_ON_STARTUP:
    text = "Loading prior state on startup";
    break;
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
  case BeaconService::State::CABINET_ROTATION:
    text = "Decide on cabinet rotation";
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

// We put this serializer in the cpp since it is serializing itself
namespace serializers {

// Note that this serializer saves the current state, and on deser will
// populate state_after_reload_
template <typename D>
struct MapSerializer<beacon::BeaconService, D>
{
public:
  using Type       = beacon::BeaconService;
  using DriverType = D;

  static uint8_t const ACTIVE_EXE_UNIT             = 1;
  static uint8_t const AEON_EXE_QUEUE              = 2;
  static uint8_t const BLOCK_ENTROPY_PREVIOUS      = 3;
  static uint8_t const BLOCK_ENTROPY_BEING_CREATED = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &beacon_service)
  {
    auto map = map_constructor(4);
    map.Append(ACTIVE_EXE_UNIT, beacon_service.active_exe_unit_);
    map.Append(AEON_EXE_QUEUE, beacon_service.aeon_exe_queue_);
    map.Append(BLOCK_ENTROPY_PREVIOUS, beacon_service.block_entropy_previous_);
    map.Append(BLOCK_ENTROPY_BEING_CREATED, beacon_service.block_entropy_being_created_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &beacon_service)
  {
    map.ExpectKeyGetValue(ACTIVE_EXE_UNIT, beacon_service.active_exe_unit_);
    map.ExpectKeyGetValue(AEON_EXE_QUEUE, beacon_service.aeon_exe_queue_);

    map.ExpectKeyGetValue(BLOCK_ENTROPY_PREVIOUS, beacon_service.block_entropy_previous_);
    map.ExpectKeyGetValue(BLOCK_ENTROPY_BEING_CREATED, beacon_service.block_entropy_being_created_);
  }
};

template <typename D>
struct MapSerializer<beacon::BeaconServiceSerializeWrapper, D>
{
public:
  using Type       = beacon::BeaconServiceSerializeWrapper;
  using DriverType = D;

  static uint8_t const BEACON_SERVICE = 1;
  static uint8_t const CURRENT_STATE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &wrapper)
  {
    auto map = map_constructor(2);
    map.Append(BEACON_SERVICE, wrapper.beacon_service);
    map.Append(CURRENT_STATE, wrapper.current_state);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &wrapper)
  {
    map.ExpectKeyGetValue(BEACON_SERVICE, wrapper.beacon_service);
    map.ExpectKeyGetValue(CURRENT_STATE, wrapper.current_state);
  }
};

}  // namespace serializers

}  // namespace fetch
