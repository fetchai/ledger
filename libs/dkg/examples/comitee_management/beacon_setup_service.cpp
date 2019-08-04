#include "beacon_setup_service.hpp"
#include "network/generics/requesting_queue.hpp"

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
  case BeaconSetupService::State::BROADCAST_ID:
    text = "Broadcasting ID";
    break;
  case BeaconSetupService::State::WAIT_FOR_IDS:
    text = "Wait for IDs";
    break;
  case BeaconSetupService::State::CREATE_SHARES:
    text = "Creating shares";
    break;
  case BeaconSetupService::State::SEND_SHARES:
    text = "Sending shares";
    break;
  case BeaconSetupService::State::WAIT_FOR_SHARES:
    text = "Waiting for shares";
    break;
  case BeaconSetupService::State::GENERATE_KEYS:
    text = "Generating keys";
    break;
  case BeaconSetupService::State::BEACON_READY:
    text = "Beacon ready";
    break;
  }

  return text;
}

BeaconSetupService::BeaconSetupService(Endpoint &endpoint, Identity identity)
  : identity_{std::move(identity)}
  , endpoint_{endpoint}
  , id_subscription_(endpoint_.Subscribe(SERVICE_DKG, CHANNEL_ID_DISTRIBUTION))
  , rpc_client_{"BeaconSetupService", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , state_machine_{std::make_shared<StateMachine>("BeaconSetupService", State::IDLE, ToString)}
{

  state_machine_->RegisterHandler(State::IDLE, this, &BeaconSetupService::OnIdle);
  state_machine_->RegisterHandler(State::BROADCAST_ID, this, &BeaconSetupService::OnBroadcastID);
  state_machine_->RegisterHandler(State::WAIT_FOR_IDS, this, &BeaconSetupService::WaitForIDs);
  state_machine_->RegisterHandler(State::CREATE_SHARES, this, &BeaconSetupService::CreateShares);
  state_machine_->RegisterHandler(State::SEND_SHARES, this, &BeaconSetupService::SendShares);
  state_machine_->RegisterHandler(State::WAIT_FOR_SHARES, this,
                                  &BeaconSetupService::OnWaitForShares);
  state_machine_->RegisterHandler(State::GENERATE_KEYS, this, &BeaconSetupService::OnGenerateKeys);
  state_machine_->RegisterHandler(State::BEACON_READY, this, &BeaconSetupService::OnBeaconReady);

  id_subscription_->SetMessageHandler(
      [this](muddle::Packet::Address const &from, uint16_t, uint16_t, uint16_t,
             muddle::Packet::Payload const &payload, muddle::Packet::Address transmitter) {
        FETCH_UNUSED(from);
        FETCH_UNUSED(transmitter);

        Serializer serialiser{payload};

        CabinetMemberDetails result;
        serialiser >> result;

        // TODO: Check signature
        // TODO: Check address
        std::lock_guard<std::mutex> lock(mutex_);
        this->member_details_queue_.emplace_back(std::move(result));
      });
}

BeaconSetupService::State BeaconSetupService::OnIdle()
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (beacon_queue_.size() > 0)
    {
      beacon_ = beacon_queue_.front();
      assert(beacon_ != nullptr);

      beacon_queue_.pop_front();
      return State::BROADCAST_ID;
    }
  }

  //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return State::IDLE;
}

BeaconSetupService::State BeaconSetupService::OnBroadcastID()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Creating identity
  CabinetMemberDetails member;
  member.identity = beacon_->manager.identity();
  member.id       = beacon_->manager.id();
  member_details_queue_.push_back(member);

  //    Serializer serializer;
  //    serializer << member.identity << member.id;
  // TODO: Sign

  Serializer msgser;
  msgser << member;
  std::cout << "Broadcasting ID" << std::endl;
  endpoint_.Broadcast(SERVICE_DKG, CHANNEL_ID_DISTRIBUTION, msgser.data());
  return State::WAIT_FOR_IDS;
}

BeaconSetupService::State BeaconSetupService::WaitForIDs()
{
  std::lock_guard<std::mutex> lock(mutex_);
  assert(beacon_ != nullptr);

  // Checking incoming messages
  std::vector<CabinetMemberDetails> remaining;
  for (auto &member : member_details_queue_)
  {
    auto it = beacon_->members.find(member.identity);
    if (it == beacon_->members.end())
    {
      remaining.push_back(member);
    }
    else
    {
      member_details_[member.identity] = member.id;
    }
  }
  member_details_queue_ = remaining;

  // Checking if we are done
  if (member_details_.size() < beacon_->members.size())
  {
    // TODO: Create strategy for missing identities.
    //      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return State::WAIT_FOR_IDS;
  }

  // Setting IDs up for the beacon
  for (auto &member : member_details_)
  {
    // TODO: Add address
    beacon_->manager.InsertMember(member.first, member.second);
  }

  return State::CREATE_SHARES;
}

BeaconSetupService::State BeaconSetupService::CreateShares()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Creating member contributions
  beacon_->manager.GenerateContribution();

  // Pre-paring to send
  for (auto &counter_party : beacon_->members)
  {
    DeliveryDetails details;
    details.was_delivered                  = false;
    share_delivery_details_[counter_party] = details;
  }

  return State::SEND_SHARES;
}

BeaconSetupService::State BeaconSetupService::SendShares()
{
  std::cout << "Sending shares" << std::endl;
  // Getting information common to all counter parties
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        verification_vector = beacon_->manager.GetVerificationVector();
    auto                        from                = beacon_->manager.identity();

    for (auto &delivery_info : share_delivery_details_)
    {
      auto &details = delivery_info.second;
      if (details.was_delivered)
      {
        continue;
      }
      auto counter_party = delivery_info.first;
      // TODO Get address
      auto share = beacon_->manager.GetShare(counter_party);

      // TODO: Add signature

      details.response =
          rpc_client_.CallSpecificAddress(counter_party.identifier(), RPC_BEACON_SETUP,
                                          SUBMIT_SHARE, from, share, verification_vector);
    }
  }

  //    std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Gettting response
  bool all_delivered = true;
  {
    /*
    // TODO: Implement
    uint32_t                    n = 0;
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &delivery_info : share_delivery_details_)
    {
      auto &details = delivery_info.second;
      if (details.was_delivered)
      {
        ++n;
        continue;
      }

      // TODO: check that response is non-null
      std::cout << "Waiting for "
      auto value = details.response->As<bool>();
      if (value)
      {
        ++n;
        details.was_delivered = true;
        continue;
      }

      all_delivered = false;
    }
    std::cout << "Delivered " << n << std::endl;
    */
  }

  if (all_delivered)
  {
    return State::WAIT_FOR_SHARES;
  }

  return State::SEND_SHARES;
}

bool BeaconSetupService::SubmitShare(Identity from, PrivateKey share,
                                     VerificationVector verification_vector)
{
  std::lock_guard<std::mutex> lock(mutex_);
  std::cout << " - Recieving share" << std::endl;
  // TODO: Check signature
  // TODO:  member is part of cabinet

  ShareSubmission submission;
  submission.from                = from;
  submission.share               = std::move(share);
  submission.verification_vector = std::move(verification_vector);

  submitted_shares_[from] = submission;

  return true;
}

BeaconSetupService::State BeaconSetupService::OnWaitForShares()
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << " -- > " << submitted_shares_.size() << std::endl;
    if (submitted_shares_.size() == beacon_->members.size())
    {
      return State::GENERATE_KEYS;
    }
  }

  std::cout << "Wait for shares" << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  return State::WAIT_FOR_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnGenerateKeys()
{
  std::cout << "Generate keys" << std::endl;
  std::lock_guard<std::mutex> lock(mutex_);

  // Adding shares
  for (auto &t : submitted_shares_)
  {
    auto &s = t.second;
    if (!beacon_->manager.AddShare(s.from, s.share, s.verification_vector))
    {
      // TODO: Serializable error
      throw std::runtime_error("share could not be verified.");
    }
  }

  // and generating secret
  beacon_->manager.CreateKeyPair();

  return State::BEACON_READY;
}

BeaconSetupService::State BeaconSetupService::OnBeaconReady()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (callback_function_)
  {
    callback_function_(beacon_);
  }

  beacon_.reset();
  return State::IDLE;
}

void BeaconSetupService::QueueSetup(SharedBeacon beacon)
{
  std::lock_guard<std::mutex> lock(mutex_);
  assert(beacon != nullptr);
  beacon_queue_.push_back(beacon);
}

// TODO: ... steps - rbc? ...
// TODO: support for complaints

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