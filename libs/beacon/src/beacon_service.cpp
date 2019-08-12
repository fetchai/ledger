#include "beacon/beacon_round.hpp"

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
  }

  return text;
}

BeaconService::BeaconService(Endpoint &endpoint, CertificatePtr certificate)
  : certificate_{certificate}
  , identity_{certificate->identity()}
  , endpoint_{endpoint}
  , state_machine_{std::make_shared<StateMachine>("BeaconService", State::WAIT_FOR_SETUP_COMPLETION,
                                                  ToString)}
  , entropy_subscription_(endpoint_.Subscribe(SERVICE_DKG, CHANNEL_ENTROPY_DISTRIBUTION))
  , rpc_client_{"BeaconService", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , cabinet_creator_{endpoint_, identity_}  // TODO: Make shared
  , cabinet_creator_protocol_{cabinet_creator_}
  , beacon_protocol_{*this}

{

  // Attaching beacon ready callback handler
  cabinet_creator_.SetBeaconReadyCallback([this](SharedBeacon beacon) {
    std::lock_guard<std::mutex> lock(mutex_);
    beacon_queue_.push_back(beacon);
  });

  // Attaching the protocol
  rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_DKG, CHANNEL_RPC);
  rpc_server_->Add(RPC_BEACON_SETUP, &cabinet_creator_protocol_);
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

  state_machine_->RegisterHandler(State::OBSERVE_ENTROPY_GENERATION, this,
                                  &BeaconService::OnObserveEntropyGeneration);
}

void BeaconService::StartNewCabinet(CabinetMemberList members, uint32_t threshold,
                                    uint64_t round_start, uint64_t round_end)
{
  std::lock_guard<std::mutex> lock(mutex_);

  SharedBeacon beacon = std::make_shared<BeaconRoundDetails>();

  if (members.find(identity_) == members.end())
  {
    beacon->observe_only = true;
  }
  else
  {
    beacon->manager.SetCertificate(certificate_);
    beacon->manager.Reset(members.size(), threshold);
  }

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

  if (active_beacon_ != nullptr)
  {
    return State::PREPARE_ENTROPY_GENERATION;
  }

  if (beacon_queue_.size() > 0)
  {
    active_beacon_ = beacon_queue_.front();
    beacon_queue_.pop_front();

    if (active_beacon_->observe_only)
    {
      return State::OBSERVE_ENTROPY_GENERATION;
    }
    else
    {
      return State::PREPARE_ENTROPY_GENERATION;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::WAIT_FOR_SETUP_COMPLETION;
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

      // TODO: Check seed
      // TODO: Verify group signature

      // and we complete.
      return State::COMPLETE;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::OBSERVE_ENTROPY_GENERATION;
}

BeaconService::State BeaconService::OnPrepareEntropyGeneration()
{
  std::lock_guard<std::mutex> lock(mutex_);

  /*
    if (next_entropy_.round >= active_beacon_.round_end)
    {
      state_machine_->Delay(std::chrono::milliseconds(500));
      return State::PREPARE_ENTROPY_GENERATION;
    }
  */
  // Finding next scheduled entropy request
  current_entropy_ = next_entropy_;

  // TODO: Check that commitee is fit to generate this entropy

  // Generating signature
  active_beacon_->manager.SetMessage(current_entropy_.seed);
  active_beacon_->member_share = active_beacon_->manager.Sign();

  return State::BROADCAST_SIGNATURE;
}

BeaconService::State BeaconService::OnBroadcastSignatureState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto &member : active_beacon_->aeon.members)
  {
    if (member == identity_)
    {
      signature_queue_.push_back({current_entropy_.round, active_beacon_->member_share});
    }
    else
    {
      rpc_client_.CallSpecificAddress(member.identifier(), RPC_BEACON,
                                      BeaconServiceProtocol::SUBMIT_SIGNATURE_SHARE,
                                      current_entropy_.round, active_beacon_->member_share);
    }
  }

  return State::COLLECT_SIGNATURES;
}

void BeaconService::SubmitSignatureShare(uint64_t round, SignatureShare share)
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (active_beacon_ == nullptr)
  {
    signature_queue_.push_back({round, share});
    return;
  }

  if ((active_beacon_->aeon.round_start <= round) && (round < active_beacon_->aeon.round_end))
  {
    if (round == current_entropy_.round)
    {
      // TODO: Refactor to AddSignatureShare(share);
      if (!active_beacon_->manager.AddSignaturePart(share.identity, share.public_key,
                                                    share.signature))
      {
        // TODO: Received invalid signature - deal with it.
        std::cout << "Signature invalid!!!" << std::endl;
      }
    }
  }
  else if (round > current_entropy_.round)
  {
    signature_queue_.push_back({round, share});
  }
}
/*
SignatureShare BeaconService::RequestSignatureShare(uint64_t round)
{
  if (active_beacon_ == nullptr)
  {
    std::cout << "TODO" << std::endl;
    return SignatureShare{};
  }

  return active_beacon_->member_share;
}
*/
BeaconService::State BeaconService::OnCollectSignaturesState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  std::deque<std::pair<uint64_t, SignatureShare>> remaining_shares;
  while (!signature_queue_.empty())
  {
    auto f = signature_queue_.front();
    signature_queue_.pop_front();
    if (f.first == current_entropy_.round)
    {
      auto &share = f.second;
      if (!active_beacon_->manager.AddSignaturePart(share.identity, share.public_key,
                                                    share.signature))
      {
        // TODO: Received invalid signature - deal with it.
        std::cout << "Signature invalid!!!" << std::endl;
      }
    }
    else if (f.first > current_entropy_.round)
    {
      remaining_shares.push_back(f);
    }
  }
  signature_queue_ = remaining_shares;

  // Checking if we can verify
  if (!active_beacon_->manager.can_verify())
  {
    state_machine_->Delay(std::chrono::milliseconds(500));
    return State::COLLECT_SIGNATURES;
    //    return State::REQUEST_MISSING_SIGNATURES;
  }

  if (active_beacon_->manager.Verify())
  {
    // Storing the result of current entropy
    current_entropy_.signature = active_beacon_->manager.GroupSignature();
    auto sign                  = crypto::bls::ToBinary(current_entropy_.signature);
    current_entropy_.entropy   = crypto::Hash<crypto::SHA256>(crypto::Hash<crypto::SHA256>(sign));

    Serializer msgser;
    msgser << current_entropy_;
    endpoint_.Broadcast(SERVICE_DKG, CHANNEL_ENTROPY_DISTRIBUTION, msgser.data());

    return State::COMPLETE;
  }

  std::cout << "SOMETHING IS WRONG!!!" << std::endl;
  return State::COMPLETE;
}

BeaconService::State BeaconService::OnCompleteState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Adding new entropy
  ready_entropy_queue_.push_back(current_entropy_);

  std::cout << "########### ..~~--=== " << byte_array::ToBase64(current_entropy_.entropy)
            << " ===--~~.. ###########" << std::endl;

  // Preparing next
  next_entropy_       = Entropy();
  next_entropy_.seed  = current_entropy_.entropy;
  next_entropy_.round = 1 + current_entropy_.round;

  return State::COMITEE_ROTATION;
}

BeaconService::State BeaconService::OnComiteeState()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (next_entropy_.round < active_beacon_->aeon.round_end)
  {
    if (active_beacon_->observe_only)
    {
      return State::OBSERVE_ENTROPY_GENERATION;
    }

    return State::PREPARE_ENTROPY_GENERATION;
  }

  // Done with the beacon.
  active_beacon_.reset();

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