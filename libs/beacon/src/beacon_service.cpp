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

#include "beacon/beacon_service.hpp"
#include "beacon/aeon.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "muddle/muddle_interface.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"

#include <chrono>
#include <iterator>

namespace fetch {
namespace beacon {

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

BeaconService::BeaconService(MuddleInterface &               muddle,
                             ledger::ManifestCacheInterface &manifest_cache,
                             CertificatePtr certificate, SharedEventManager event_manager,
                             uint64_t blocks_per_round)
  : certificate_{certificate}
  , identity_{certificate->identity()}
  , endpoint_{muddle.GetEndpoint()}
  , state_machine_{std::make_shared<StateMachine>("BeaconService", State::WAIT_FOR_SETUP_COMPLETION,
                                                  ToString)}
  , blocks_per_round_{blocks_per_round}
  , group_public_key_subscription_{endpoint_.Subscribe(SERVICE_DKG, CHANNEL_PUBLIC_KEY)}
  , entropy_subscription_{endpoint_.Subscribe(SERVICE_DKG, CHANNEL_ENTROPY_DISTRIBUTION)}
  , rpc_client_{"BeaconService", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , event_manager_{std::move(event_manager)}
  , cabinet_creator_{muddle, identity_, manifest_cache}  // TODO(tfr): Make shared
  , beacon_protocol_{*this}
  , beacon_entropy_generated_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_entropy_generated_total", "The total number of times entropy has been generated")}
  , beacon_entropy_future_signature_seen_total_{telemetry::Registry::Instance().CreateCounter(
        "beacon_entropy_future_signature_seen_total",
        "The total number of times entropy has been generated")}
  , beacon_entropy_last_requested_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "beacon_entropy_last_requested", "The last entropy value requested from the beacon")}
{

  // Attaching beacon ready callback handler
  cabinet_creator_.SetBeaconReadyCallback([this](SharedAeonExecutionUnit beacon) {
    std::lock_guard<std::mutex> lock(mutex_);
    aeon_exe_queue_.push_back(beacon);
  });

  // Attaching the protocol
  rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_DKG, CHANNEL_RPC);
  rpc_server_->Add(RPC_BEACON, &beacon_protocol_);

  // Subcribing to incoming entropy
  entropy_subscription_->SetMessageHandler(
      [this](muddle::Packet::Address const &from, uint16_t, uint16_t, uint16_t,
             muddle::Packet::Payload const &payload, muddle::Packet::Address transmitter) {
        FETCH_UNUSED(from);
        FETCH_UNUSED(transmitter);

        Serializer serialiser{payload};

        Entropy result;
        serialiser >> result;

        std::lock_guard<std::mutex> lock(mutex_);
        this->incoming_entropy_.emplace(std::move(result));
      });

  group_public_key_subscription_->SetMessageHandler(
      [this](muddle::Packet::Address const &from, uint16_t, uint16_t, uint16_t,
             muddle::Packet::Payload const &payload, muddle::Packet::Address transmitter) {
        FETCH_UNUSED(from);
        FETCH_UNUSED(transmitter);

        PublicKeyMessage result;
        Serializer       serialiser{payload};
        serialiser >> result;

        std::lock_guard<std::mutex> lock(mutex_);
        this->incoming_group_public_keys_.emplace(std::move(result));
      });

  // Pipe 1
  state_machine_->RegisterHandler(State::WAIT_FOR_SETUP_COMPLETION, this,
                                  &BeaconService::OnWaitForSetupCompletionState);
  state_machine_->RegisterHandler(State::PREPARE_ENTROPY_GENERATION, this,
                                  &BeaconService::OnPrepareEntropyGeneration);
  state_machine_->RegisterHandler(State::COLLECT_SIGNATURES, this,
                                  &BeaconService::OnCollectSignaturesState);
  state_machine_->RegisterHandler(State::VERIFY_SIGNATURES, this,
                                  &BeaconService::OnVerifySignaturesState);
  state_machine_->RegisterHandler(State::COMPLETE, this, &BeaconService::OnCompleteState);
  state_machine_->RegisterHandler(State::COMITEE_ROTATION, this, &BeaconService::OnComiteeState);

  // Pipe 2
  state_machine_->RegisterHandler(State::WAIT_FOR_PUBLIC_KEYS, this,
                                  &BeaconService::OnWaitForPublicKeys);
  state_machine_->RegisterHandler(State::OBSERVE_ENTROPY_GENERATION, this,
                                  &BeaconService::OnObserveEntropyGeneration);

  state_machine_->OnStateChange([this](State current, State previous) {
    FETCH_UNUSED(this);
    FETCH_UNUSED(current);
    FETCH_UNUSED(previous);
    FETCH_LOG_DEBUG(LOGGING_NAME, "Current state: ", ToString(current),
                    " (previous: ", ToString(previous), ")");
  });
}

BeaconService::Status BeaconService::GenerateEntropy(Digest /*block_digest*/, uint64_t block_number,
                                                     uint64_t &entropy)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Requesting entropy for block number: ", block_number);
  beacon_entropy_last_requested_->set(block_number);

  uint64_t round = block_number / blocks_per_round_;

  if (latest_entropy_.round == round && latest_entropy_)
  {
    entropy = latest_entropy_.EntropyAsUint64();
    return Status::OK;
  }

  // Searches for the next entropy
  do
  {
    if (ready_entropy_queue_.size() == 0)
    {
      return Status::NOT_READY;
    }

    latest_entropy_ = ready_entropy_queue_.front();
    ready_entropy_queue_.pop_front();
  } while (latest_entropy_.round < round);

  // TODO(tfr): Roll support does not exist yet.
  if (round < latest_entropy_.round)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "No support for roll back yet.");
    return Status::FAILED;
  }

  // We found the entropy
  assert(latest_entropy_);
  entropy = latest_entropy_.EntropyAsUint64();

  return Status::OK;
}

void BeaconService::AbortCabinet(uint64_t round_start)
{
  cabinet_creator_.Abort(round_start);
}

void BeaconService::StartNewCabinet(CabinetMemberList members, uint32_t threshold,
                                    uint64_t round_start, uint64_t round_end, uint64_t start_time)
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

  std::lock_guard<std::mutex> lock(mutex_);

  SharedAeonExecutionUnit beacon = std::make_shared<AeonExecutionUnit>();

  // Determines if we are observing or actively participating
  if (members.find(identity_) == members.end())
  {
    beacon->observe_only = true;
    FETCH_LOG_INFO(LOGGING_NAME, "Beacon in observe only mode. Members: ", members.size());
  }
  else
  {
    beacon->manager.SetCertificate(certificate_);
    beacon->manager.Reset(members, threshold);
  }

  // Setting the aeon details
  beacon->aeon.round_start               = round_start;
  beacon->aeon.round_end                 = round_end;
  beacon->aeon.members                   = std::move(members);
  beacon->aeon.start_reference_timepoint = start_time;

  // Even "observe only" details need to pass through the setup phase
  // to preserve order.
  cabinet_creator_.QueueSetup(beacon);
}

BeaconService::State BeaconService::OnWaitForSetupCompletionState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (active_exe_unit_ != nullptr)
  {
    return State::PREPARE_ENTROPY_GENERATION;
  }

  // Checking whether the next committee is ready
  // to produce random numbers.
  if (aeon_exe_queue_.size() > 0)
  {
    active_exe_unit_ = aeon_exe_queue_.front();
    aeon_exe_queue_.pop_front();

    // We distinguish between those members that
    // observe the process
    if (active_exe_unit_->observe_only)
    {
      return State::WAIT_FOR_PUBLIC_KEYS;
    }
    else
    {
      // and those who run it.
      Serializer       msgser;
      PublicKeyMessage pk;

      pk.round = active_exe_unit_->aeon.round_start;
      pk.group_public_key.setStr(active_exe_unit_->manager.group_public_key());

      msgser << pk;
      endpoint_.Broadcast(SERVICE_DKG, CHANNEL_PUBLIC_KEY, msgser.data());

      return State::PREPARE_ENTROPY_GENERATION;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::WAIT_FOR_SETUP_COMPLETION;
}

BeaconService::State BeaconService::OnWaitForPublicKeys()
{
  std::lock_guard<std::mutex> lock(mutex_);

  while (incoming_group_public_keys_.size() > 0)
  {
    auto pk = incoming_group_public_keys_.top();
    incoming_group_public_keys_.pop();

    // Skipping all which belong to previous ronds
    if (pk.round < active_exe_unit_->aeon.round_start)
    {
      continue;
    }

    // Creating the public key
    FETCH_LOG_INFO(LOGGING_NAME, "Received group key");
    if (pk.round == active_exe_unit_->aeon.round_start)
    {
      active_exe_unit_->manager.SetGroupPublicKey(pk.group_public_key);
      return State::OBSERVE_ENTROPY_GENERATION;
    }
  }

  return State::WAIT_FOR_PUBLIC_KEYS;
}

BeaconService::State BeaconService::OnObserveEntropyGeneration()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Iterating through incoming
  while (incoming_entropy_.size() > 0)
  {
    current_entropy_ = incoming_entropy_.top();

    // Skipping entropy from previous rounds
    if (next_entropy_.round > current_entropy_.round)
    {
      incoming_entropy_.pop();
      continue;
    }

    // If the round matches what we expect ...
    if (next_entropy_.round == current_entropy_.round)
    {
      incoming_entropy_.pop();

      // ... the validity is verified
      assert(next_entropy_.seed == current_entropy_.seed);
      assert(active_exe_unit_->observe_only);
      active_exe_unit_->manager.SetMessage(next_entropy_.seed);
      if (!active_exe_unit_->manager.Verify(current_entropy_.signature))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Verification of entropy for round ", current_entropy_.round,
                       " failed");
        // Creating event for incorrect signature
        EventInvalidSignature event;
        // TODO(tfr): Received invalid signature - fill event details
        event_manager_->Dispatch(event);

        continue;
      }

      // and we complete.
      FETCH_LOG_INFO(LOGGING_NAME, "Verification of entropy for round ", current_entropy_.round,
                     " completed");
      return State::COMPLETE;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(100));
  return State::OBSERVE_ENTROPY_GENERATION;
}

BeaconService::State BeaconService::OnPrepareEntropyGeneration()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Check that commitee is fit to generate this entropy
  if (!((active_exe_unit_->aeon.round_start <= next_entropy_.round) &&
        (next_entropy_.round < active_exe_unit_->aeon.round_end)))
  {
    FETCH_LOG_CRITICAL(LOGGING_NAME,
                       "Wrong committee instated for round - this should not happen.");
    // TODO(tfr): Work out how this should be dealt with
    active_exe_unit_.reset();

    return State::WAIT_FOR_SETUP_COMPLETION;
  }

  // Finding next scheduled entropy request
  current_entropy_ = next_entropy_;

  // Generating signature
  active_exe_unit_->manager.SetMessage(current_entropy_.seed);
  active_exe_unit_->member_share = active_exe_unit_->manager.Sign();

  // Ready to broadcast signatures
  return State::COLLECT_SIGNATURES;
}

/**
 * Peers can call this function (RPC endpoint) to get threshold signatures that
 * this peer has collected
 */
BeaconService::SignatureInformation BeaconService::GetSignatureShares(uint64_t round)
{
  std::lock_guard<std::mutex> lock(mutex_);

  // If this is too far in the future or the past return empty struct
  if (signatures_being_built_.find(round) == signatures_being_built_.end())
  {
    return {};
  }

  return signatures_being_built_.at(round);
}

BeaconService::State BeaconService::OnCollectSignaturesState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // On first entry to function, populate with our info
  if (state_machine_->previous_state() == State::PREPARE_ENTROPY_GENERATION)
  {
    SignatureInformation this_round;
    this_round.round                                        = current_entropy_.round;
    this_round.threshold_signatures[identity_.identifier()] = active_exe_unit_->member_share;
    signatures_being_built_[current_entropy_.round]         = this_round;

    // TODO(HUT): clean historically old sigs here
  }

  // Attempt to get signatures from a peer we do not have the signature of
  auto        missing_signatures_from = active_exe_unit_->manager.qual();
  auto const &signatures_struct       = signatures_being_built_[current_entropy_.round];

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
  sig_share_promise_     = rpc_client_.CallSpecificAddress(
      qual_promise_identity_.identifier(), RPC_BEACON, BeaconServiceProtocol::GET_SIGNATURE_SHARES,
      current_entropy_.round);

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

  // Don't lock until the promise has resolved! Otherwise the system can deadlock.
  std::lock_guard<std::mutex> lock(mutex_);

  if (ret.threshold_signatures.empty())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Peer wasn't ready when asking for signatures: ",
                   qual_promise_identity_.identifier().ToBase64());
    state_machine_->Delay(std::chrono::milliseconds(10));
    return State::COLLECT_SIGNATURES;
  }

  if (ret.round != current_entropy_.round)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Peer returned the wrong round when asked for signatures. Peer: ",
                   qual_promise_identity_.identifier().ToBase64(), " returned: ", ret.round,
                   " expected: ", current_entropy_.round);
    return State::COLLECT_SIGNATURES;
  }

  // Success - Add relevant info
  auto &signatures_struct = signatures_being_built_[current_entropy_.round];
  auto &all_sigs_map      = signatures_struct.threshold_signatures;

  for (auto const &address_sig_pair : ret.threshold_signatures)
  {
    all_sigs_map[address_sig_pair.first] = address_sig_pair.second;
    // Let the manager know
    AddSignature(address_sig_pair.second);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "After adding, we have ", all_sigs_map.size(),
                 " signatures. Round: ", current_entropy_.round);

  if (active_exe_unit_->manager.can_verify() && active_exe_unit_->manager.Verify())
  {
    // Storing the result of current entropy
    current_entropy_.signature = active_exe_unit_->manager.GroupSignature();
    auto sign                  = current_entropy_.signature.getStr();
    current_entropy_.entropy   = crypto::Hash<crypto::SHA256>(crypto::Hash<crypto::SHA256>(sign));

    // Broadcasting the entropy to those listening, but not participating
    if (broadcasting_)
    {
      Serializer msgser;
      msgser << current_entropy_;
      endpoint_.Broadcast(SERVICE_DKG, CHANNEL_ENTROPY_DISTRIBUTION, msgser.data());
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Generated new entropy value");

    return State::COMPLETE;
  }
  else
  {
    return State::COLLECT_SIGNATURES;
  }
}

BeaconService::State BeaconService::OnCompleteState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Adding new entropy
  ready_entropy_queue_.push_back(current_entropy_);
  beacon_entropy_generated_total_->add(1);

  // Preparing next
  next_entropy_       = Entropy();
  next_entropy_.seed  = current_entropy_.entropy;
  next_entropy_.round = 1 + current_entropy_.round;

  return State::COMITEE_ROTATION;
}

BeaconService::State BeaconService::OnComiteeState()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (next_entropy_.round < active_exe_unit_->aeon.round_end)
  {
    if (active_exe_unit_->observe_only)
    {
      return State::OBSERVE_ENTROPY_GENERATION;
    }

    return State::PREPARE_ENTROPY_GENERATION;
  }

  // Dispatching
  EventCommitteeCompletedWork event;
  event.aeon = active_exe_unit_->aeon;
  event_manager_->Dispatch(event);

  // Done with the beacon.
  active_exe_unit_.reset();

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
  else if (ret == BeaconManager::AddResult::NOT_MEMBER)
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

std::weak_ptr<core::Runnable> BeaconService::GetMainRunnable()
{
  return state_machine_;
}

std::weak_ptr<core::Runnable> BeaconService::GetSetupRunnable()
{
  return cabinet_creator_.GetWeakRunnable();
}

}  // namespace beacon
}  // namespace fetch
