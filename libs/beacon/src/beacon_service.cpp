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
  }

  return text;
}

BeaconService::BeaconService(Endpoint &endpoint, CertificatePtr certificate)
  : certificate_{certificate}
  , identity_{certificate->identity()}
  , endpoint_{endpoint}
  , state_machine_{std::make_shared<StateMachine>("BeaconService", State::WAIT_FOR_SETUP_COMPLETION,
                                                  ToString)}
  , rpc_client_{"BeaconService", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , cabinet_creator_{endpoint_, identity_}  // TODO: Make shared
  , cabinet_creator_protocol_{cabinet_creator_}
  , beacon_protocol_{*this}

{
  // TODO: Make configurable
  Entropy start_entropy;
  entropy_queue_.push_back(start_entropy);

  // Attaching beacon ready callback handler
  cabinet_creator_.SetBeaconReadyCallback([this](SharedBeacon beacon) {
    std::lock_guard<std::mutex> lock(mutex_);
    beacon_queue_.push_back(beacon);
  });

  // Attaching the protocol
  rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_DKG, CHANNEL_RPC);
  rpc_server_->Add(RPC_BEACON_SETUP, &cabinet_creator_protocol_);
  rpc_server_->Add(RPC_BEACON, &beacon_protocol_);

  // BeaconService::State machine:
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
}

void BeaconService::StartNewCabinet(CabinetMemberList members, uint32_t threshold,
                                    uint64_t round_start, uint64_t round_end)
{
  std::lock_guard<std::mutex> lock(mutex_);

  // TODO: Separate the details from the managing components
  SharedBeacon beacon = std::make_shared<BeaconRoundDetails>();

  beacon->manager.SetCertificate(certificate_);
  beacon->manager.Reset(members.size(), threshold);
  beacon->round_start = round_start;
  beacon->round_end   = round_end;
  beacon->members     = std::move(members);
  assert(beacon != nullptr);

  cabinet_creator_.QueueSetup(beacon);
}

void BeaconService::SkipRound()
{
  std::lock_guard<std::mutex> lock(mutex_);
  ++next_cabinet_generation_number_;
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
    return State::PREPARE_ENTROPY_GENERATION;
  }

  return State::WAIT_FOR_SETUP_COMPLETION;
}

BeaconService::State BeaconService::OnPrepareEntropyGeneration()
{
  std::lock_guard<std::mutex> lock(mutex_);

  if (entropy_queue_.empty())
  {
    state_machine_->Delay(std::chrono::milliseconds(500));
    return State::PREPARE_ENTROPY_GENERATION;
  }

  // Finding next scheduled entropy request
  current_entropy_ = entropy_queue_.front();
  entropy_queue_.pop_front();

  // Generating signature
  active_beacon_->manager.SetMessage(current_entropy_.seed);
  active_beacon_->member_share = active_beacon_->manager.Sign();

  return State::BROADCAST_SIGNATURE;
}

BeaconService::State BeaconService::OnBroadcastSignatureState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto &member : active_beacon_->members)
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

  if (round == current_entropy_.round)
  {
    // TODO: Refactor to AddSignatureShare(share);
    if (!active_beacon_->manager.AddSignaturePart(share.identity, share.public_key,
                                                  share.signature))
    {
      // TODO: Received invalid signature - deal with it.
      std::cout << "Signature invalid!!!" << std::endl;
    }

    // Storing signature
    current_entropy_.signatures.push_back(share);
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
  std::cout << "COLLECTING SIGNATURES" << std::endl;
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

      // Storing signature
      current_entropy_.signatures.push_back(share);
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
    return State::COMPLETE;
  }

  std::cout << "SOMETHING IS WRONG!!!" << std::endl;
  return State::COMPLETE;
}

BeaconService::State BeaconService::OnCompleteState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Storing the result of current entropy
  current_entropy_.entropy = crypto::Hash<crypto::SHA256>(
      crypto::Hash<crypto::SHA256>(active_beacon_->manager.GroupSignature()));
  ready_entropy_queue_.push_back(current_entropy_);

  std::cout << "########### ..~~--=== " << byte_array::ToBase64(current_entropy_.entropy)
            << " ===--~~.. ###########" << std::endl;

  // Preparing next

  next_entropy.seed  = current_entropy_.entropy;
  next_entropy.round = 1 + current_entropy_.round;

  entropy_queue_.push_back(next_entropy);  // TODO: Get rid of queue

  return State::COMITEE_ROTATION;
}

BeaconService::State BeaconService::OnComiteeState()
{
  // TODO: Check if commitee needs changing
  return State::WAIT_FOR_SETUP_COMPLETION;
}

void BeaconService::ScheduleEntropyGeneration()
{
  Entropy next_entropy = entropy_queue_.back();
  ++next_entropy.number;
  if (next_entropy.number > 30)
  {
    next_entropy.number = 0;
    ++next_entropy.round;
  }
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