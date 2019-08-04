#include "beacon_service.hpp"

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
  , cabinet_creator_{endpoint_, identity_}  // TODO: Make shared
  , cabinet_creator_protocol_{cabinet_creator_}
  , beacon_protocol_{*this}

{
  // TODO: Make configurable
  Entropy start_entropy;
  entropy_queue_.push_back(start_entropy);

  // Attaching beacon ready callback handler
  cabinet_creator_.SetBeaconReadyCallback([this](SharedBeacon beacon) {
    std::cout << "Beacon is ready!" << std::endl;
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
                                  &BeaconService::OnBroadcastSignatureState);
  state_machine_->RegisterHandler(State::BROADCAST_SIGNATURE, this,
                                  &BeaconService::OnBroadcastSignatureState);
  state_machine_->RegisterHandler(State::COLLECT_SIGNATURES, this,
                                  &BeaconService::OnCollectSignaturesState);
  state_machine_->RegisterHandler(State::COMPLETE, this, &BeaconService::OnCompleteState);
  state_machine_->RegisterHandler(State::COMITEE_ROTATION, this, &BeaconService::OnComiteeState);
}

void BeaconService::StartNewCabinet(CabinetMemberList members, uint32_t threshold)
{
  std::lock_guard<std::mutex> lock(mutex_);

  SharedBeacon beacon = std::make_shared<BeaconRoundDetails>();

  beacon->manager.SetCertificate(certificate_);
  beacon->manager.Reset(members.size(), threshold);
  beacon->round   = next_cabinet_generation_number_;
  beacon->members = std::move(members);
  assert(beacon != nullptr);

  cabinet_creator_.QueueSetup(beacon);
  ++next_cabinet_generation_number_;
}

void BeaconService::SkipRound()
{
  std::lock_guard<std::mutex> lock(mutex_);
  ++next_cabinet_generation_number_;
}

bool BeaconService::SwitchCabinet()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (beacon_queue_.size() == 0)
  {
    return false;
  }

  auto f = beacon_queue_.front();
  active_beacon_.reset();

  if (f->round == next_cabinet_number_)
  {
    active_beacon_ = f;
  }

  ++next_cabinet_number_;
  return true;
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
    std::cout << "Queue size: " << beacon_queue_.size() << std::endl;
    active_beacon_ = beacon_queue_.front();
    beacon_queue_.pop_front();
    return State::PREPARE_ENTROPY_GENERATION;
  }

  return State::WAIT_FOR_SETUP_COMPLETION;
}

BeaconService::State BeaconService::OnPrepareEntropyGeneration()
{
  std::cout << "Preparing entropy generation" << std::endl;
  std::lock_guard<std::mutex> lock(mutex_);
  if (entropy_queue_.empty())
  {
    return State::PREPARE_ENTROPY_GENERATION;
  }

  // Finding next piece entropy
  do
  {
    // Adding the last generated entropy to the extraction queue
    ready_entropy_queue_.push_back(current_entropy_);

    // Finding next scheduled entropy request
    current_entropy_ = entropy_queue_.front();
    entropy_queue_.pop_front();
  } while ((!entropy_queue_.empty()) && (current_entropy_.round < active_beacon_->round));

  if (entropy_queue_.empty())
  {
    return State::PREPARE_ENTROPY_GENERATION;
  }

  // Generating signature
  active_beacon_->manager.SetMessage(current_entropy_.seed);
  active_beacon_->member_share = active_beacon_->manager.Sign();

  return State::BROADCAST_SIGNATURE;
}

BeaconService::State BeaconService::OnBroadcastSignatureState()
{
  /*
  for (auto &member : active_beacon_->members)
  {
    rpc_client_.CallSpecificAddress(member.identifier(), RPC_, SUBMIT_SIGNATURE,
                                    current_entropy_.round, active_beacon_->member_share);
  }
  */
  return State::COLLECT_SIGNATURES;
}

void BeaconService::SubmitSignatureShare(uint64_t /*round*/, uint64_t /*number*/,
                                         SignatureShare /*share*/)
{}

BeaconService::State BeaconService::OnCollectSignaturesState()
{

  return State::COMPLETE;
}

BeaconService::State BeaconService::OnCompleteState()
{

  return State::COMITEE_ROTATION;
}

BeaconService::State BeaconService::OnComiteeState()
{

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