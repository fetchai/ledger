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

#include <chrono>

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
  case BeaconService::State::BROADCAST_SIGNATURE:
    text = "Broadcasting signatures";
    break;
  case BeaconService::State::COLLECT_SIGNATURES:
    text = "Collecting signatures";
    break;
  case BeaconService::State::COMPLETE:
    text = "Sending shares";
    break;
  case BeaconService::State::COMITEE_ROTATION:
    text = "Waiting for shares";
    break;
  case BeaconService::State::OBSERVE_ENTROPY_GENERATION:
    text = "Observe entropy generation";
    break;
  case BeaconService::State::WAIT_FOR_VERIFICATION_VECTORS:
    text = "Waiting for verification vectors.";
    break;
  }

  return text;
}

BeaconService::BeaconService(Endpoint &endpoint, CertificatePtr certificate,
                             SharedEventManager event_manager, uint64_t blocks_per_round)
  : certificate_{certificate}
  , identity_{certificate->identity()}
  , endpoint_{endpoint}
  , state_machine_{std::make_shared<StateMachine>("BeaconService", State::WAIT_FOR_SETUP_COMPLETION,
                                                  ToString)}
  , blocks_per_round_{blocks_per_round}
  , group_public_key_subscription_{endpoint_.Subscribe(SERVICE_DKG, CHANNEL_PUBLIC_KEY)}
  , entropy_subscription_{endpoint_.Subscribe(SERVICE_DKG, CHANNEL_ENTROPY_DISTRIBUTION)}
  , rpc_client_{"BeaconService", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , event_manager_{std::move(event_manager)}
  , cabinet_creator_{endpoint_, identity_}  // TODO(tfr): Make shared
  , beacon_protocol_{*this}

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
  state_machine_->RegisterHandler(State::BROADCAST_SIGNATURE, this,
                                  &BeaconService::OnBroadcastSignatureState);
  state_machine_->RegisterHandler(State::COLLECT_SIGNATURES, this,
                                  &BeaconService::OnCollectSignaturesState);
  state_machine_->RegisterHandler(State::COMPLETE, this, &BeaconService::OnCompleteState);
  state_machine_->RegisterHandler(State::COMITEE_ROTATION, this, &BeaconService::OnComiteeState);

  // Pipe 2
  state_machine_->RegisterHandler(State::WAIT_FOR_VERIFICATION_VECTORS, this,
                                  &BeaconService::OnWaitForVerificationVectors);
  state_machine_->RegisterHandler(State::OBSERVE_ENTROPY_GENERATION, this,
                                  &BeaconService::OnObserveEntropyGeneration);
}

BeaconService::Status BeaconService::GenerateEntropy(Digest /*block_digest*/, uint64_t block_number,
                                                     uint64_t &entropy)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Generating entropy");

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

void BeaconService::StartNewCabinet(CabinetMemberList members, uint32_t threshold,
                                    uint64_t round_start, uint64_t round_end)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting new cabinet from ", round_start, " to ", round_end);
  // TODO(jmw): Fix deadlock here
  // std::lock_guard<std::mutex> lock(mutex_);

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
    // TODO(jmw): Fix
    std::set<ConstByteArray> cabinet;
    for (auto &m : members)
    {
      cabinet.insert(m.identifier());
    }
    beacon->manager.Reset(cabinet, threshold);
  }

  // Setting the aeon details
  beacon->aeon.round_start = round_start;
  beacon->aeon.round_end   = round_end;
  beacon->aeon.members     = std::move(members);

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
      return State::WAIT_FOR_VERIFICATION_VECTORS;
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

BeaconService::State BeaconService::OnWaitForVerificationVectors()
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

  return State::WAIT_FOR_VERIFICATION_VECTORS;
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
  return State::BROADCAST_SIGNATURE;
}

BeaconService::State BeaconService::OnBroadcastSignatureState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto &member : active_exe_unit_->aeon.members)
  {
    if (member == identity_)
    {
      signature_queue_.push_back({current_entropy_.round, active_exe_unit_->member_share});
    }
    else
    {
      // Communicating signatures
      rpc_client_.CallSpecificAddress(member.identifier(), RPC_BEACON,
                                      BeaconServiceProtocol::SUBMIT_SIGNATURE_SHARE,
                                      current_entropy_.round, active_exe_unit_->member_share);
      // TODO(tfr): Handle events that time out?
    }
  }

  return State::COLLECT_SIGNATURES;
}

void BeaconService::SubmitSignatureShare(uint64_t round, SignatureShare share)
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Late messages are ignored
  if (round < current_entropy_.round)
  {
    return;
  }

  // This is to ensure that the system remains operational even
  // if no active unit is running
  if (active_exe_unit_ == nullptr)
  {
    signature_queue_.push_back({round, share});
    return;
  }

  // In case the active unit
  if ((active_exe_unit_->aeon.round_start <= round) && (round < active_exe_unit_->aeon.round_end))
  {
    if (round == current_entropy_.round)
    {
      AddSignature(share);
    }
  }
  else if (round > current_entropy_.round)
  {
    // Otherwise it is stored to be dealt with at a later point.
    signature_queue_.push_back({round, share});
  }
}

BeaconService::State BeaconService::OnCollectSignaturesState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Adding signatures from the queue
  std::deque<std::pair<uint64_t, SignatureShare>> remaining_shares;
  while (!signature_queue_.empty())
  {
    auto f = signature_queue_.front();
    signature_queue_.pop_front();
    if (f.first == current_entropy_.round)
    {
      AddSignature(f.second);
    }
    else if (f.first > current_entropy_.round)
    {
      remaining_shares.push_back(f);
    }
  }
  signature_queue_ = remaining_shares;

  // Checking if we can verify
  if (!active_exe_unit_->manager.can_verify())
  {
    state_machine_->Delay(std::chrono::milliseconds(200));
    return State::BROADCAST_SIGNATURE;
  }

  if (active_exe_unit_->manager.Verify())
  {
    // Storing the result of current entropy
    current_entropy_.signature = active_exe_unit_->manager.GroupSignature();
    auto sign                  = current_entropy_.signature.getStr();
    current_entropy_.entropy   = crypto::Hash<crypto::SHA256>(crypto::Hash<crypto::SHA256>(sign));

    // Broadcasting the entropy to those listening, but not participating
    Serializer msgser;
    msgser << current_entropy_;
    endpoint_.Broadcast(SERVICE_DKG, CHANNEL_ENTROPY_DISTRIBUTION, msgser.data());

    return State::COMPLETE;
  }

  FETCH_LOG_CRITICAL(
      LOGGING_NAME,
      "Valid signatures have generated an invalid group signature - this should not happen.");
  // TODO(tfr): Work out how this should be dealt with
  return State::COMPLETE;
}

BeaconService::State BeaconService::OnCompleteState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Adding new entropy
  ready_entropy_queue_.push_back(current_entropy_);

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
