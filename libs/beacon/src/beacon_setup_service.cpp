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

#include "beacon/beacon_setup_protocol.hpp"
#include "beacon/beacon_setup_service.hpp"
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
  case BeaconSetupService::State::WAIT_FOR_DIRECT_CONNECTIONS:
    text = "Waiting for direct connections";
    break;
  case BeaconSetupService::State::WAIT_FOR_IDS:
    text = "Wait for IDs";
    break;
  case BeaconSetupService::State::CREATE_SHARES:
    text = "Creating shares";
    break;
  case BeaconSetupService::State::WAIT_FOR_VERIFICATION_VECTORS:
    text = "Waiting for verfication vectors";
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
  state_machine_->RegisterHandler(State::WAIT_FOR_DIRECT_CONNECTIONS, this,
                                  &BeaconSetupService::OnWaitForDirectConnections);
  state_machine_->RegisterHandler(State::BROADCAST_ID, this, &BeaconSetupService::OnBroadcastID);
  state_machine_->RegisterHandler(State::WAIT_FOR_IDS, this, &BeaconSetupService::WaitForIDs);
  state_machine_->RegisterHandler(State::CREATE_SHARES, this, &BeaconSetupService::CreateShares);
  state_machine_->RegisterHandler(State::WAIT_FOR_VERIFICATION_VECTORS, this,
                                  &BeaconSetupService::WaitForVerificationVectors);

  state_machine_->RegisterHandler(State::SEND_SHARES, this, &BeaconSetupService::SendShares);
  state_machine_->RegisterHandler(State::WAIT_FOR_SHARES, this,
                                  &BeaconSetupService::OnWaitForShares);
  state_machine_->RegisterHandler(State::GENERATE_KEYS, this, &BeaconSetupService::OnGenerateKeys);
  state_machine_->RegisterHandler(State::BEACON_READY, this, &BeaconSetupService::OnBeaconReady);

  // Setting the RBC protocol up for verification vectors
  broadcast_protocol_.reset(new ReliableBroadcastProt(
      endpoint_, identity_.identifier(),
      [this](network::RBC::MuddleAddress const &address, ConstByteArray const &payload) -> void {
        VerificationVector verification_vector;
        Serializer         serialiser{payload};
        serialiser >> verification_vector;

        std::lock_guard<std::mutex> lock(mutex_);
        member_verification_vector_[address] = verification_vector;
      },
      CHANNEL_VERIFICATION_VECTORS));

  // Setting the ID subscription handler up
  id_subscription_->SetMessageHandler(
      [this](muddle::Packet::Address const &from, uint16_t, uint16_t, uint16_t,
             muddle::Packet::Payload const &payload, muddle::Packet::Address transmitter) {
        FETCH_UNUSED(from);
        FETCH_UNUSED(transmitter);

        Serializer serialiser{payload};

        CabinetMemberDetails result;
        serialiser >> result;

        // TODO(tfr): Check signature
        // TODO(tfr): Check address
        std::lock_guard<std::mutex> lock(mutex_);
        this->member_details_queue_.emplace_back(std::move(result));
      });
}

BeaconSetupService::State BeaconSetupService::OnIdle()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (aeon_exe_queue_.size() > 0)
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
      // Creating broadcast protocol
      // TODO: Need support to identify malicious activity
      std::set<ConstByteArray> cabinet;
      for (auto &x : beacon_->aeon.members)
      {
        cabinet.insert(x.identifier());
      }
      broadcast_protocol_->ResetCabinet(cabinet);

      // Initiating setup
      return State::WAIT_FOR_DIRECT_CONNECTIONS;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(100));
  return State::IDLE;
}

BeaconSetupService::State BeaconSetupService::OnWaitForDirectConnections()
{
  std::lock_guard<std::mutex> lock(mutex_);
  auto                        v_peers = endpoint_.GetDirectlyConnectedPeers();
  std::unordered_set<Address> peers(v_peers.begin(), v_peers.end());

  bool     all_connected = true;
  uint16_t connected     = 0;

  for (auto &m : beacon_->aeon.members)
  {
    // Skipping own address
    if (m == identity_)
    {
      continue;
    }

    // Checking if other peers are there
    if (peers.find(m.identifier()) == peers.end())
    {
      all_connected = false;

      // TODO(tfr): Request muddle to connect.
    }
    else
    {
      connected++;
    }
  }

  if (all_connected)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "All peers connected. Proceeding.");
    return State::BROADCAST_ID;
  }

  state_machine_->Delay(std::chrono::milliseconds(200));
  FETCH_LOG_INFO(LOGGING_NAME,
                 "Waiting for all peers to join before starting setup. Connected: ", connected,
                 " expect: ", beacon_->aeon.members.size() - 1);

  return State::WAIT_FOR_DIRECT_CONNECTIONS;
}

BeaconSetupService::State BeaconSetupService::OnBroadcastID()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Creating identity
  CabinetMemberDetails member;
  member.identity = beacon_->manager.identity();
  member.id       = beacon_->manager.id();
  member_details_queue_.push_back(member);

  Serializer msgser;
  msgser << member;

  // TODO(tfr): Require signed connection
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
    auto it = beacon_->aeon.members.find(member.identity);
    if (it == beacon_->aeon.members.end())
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
  if (member_details_.size() < beacon_->aeon.members.size())
  {
    // TODO(tfr): Create strategy for missing identities.
    //      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return State::WAIT_FOR_IDS;
  }

  // Setting IDs up for the beacon
  for (auto &member : member_details_)
  {
    // TODO(tfr): Add address
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
  for (auto &counter_party : beacon_->aeon.members)
  {
    DeliveryDetails details;
    details.was_delivered                  = false;
    share_delivery_details_[counter_party] = details;
  }

  // Broadcasting verification shares
  auto       verification_vector = beacon_->manager.GetVerificationVector();
  Serializer serialiser;
  serialiser << verification_vector;
  broadcast_protocol_->Broadcast(serialiser.data());
  member_verification_vector_[identity_.identifier()] = verification_vector;

  return State::WAIT_FOR_VERIFICATION_VECTORS;
}

BeaconSetupService::State BeaconSetupService::WaitForVerificationVectors()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (member_verification_vector_.size() != beacon_->aeon.members.size())
  {
    FETCH_LOG_INFO(LOGGING_NAME,
                   "Waiting for verification vectors: ", member_verification_vector_.size(),
                   " expect: ", beacon_->aeon.members.size());
    state_machine_->Delay(std::chrono::milliseconds(100));
    return State::WAIT_FOR_VERIFICATION_VECTORS;
  }

  return State::SEND_SHARES;
}

BeaconSetupService::State BeaconSetupService::SendShares()
{
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
      auto share         = beacon_->manager.GetShare(counter_party);

      if (counter_party == identity_)
      {
        ShareSubmission submission;
        submission.from  = from;
        submission.share = std::move(share);

        submitted_shares_[from] = submission;
      }
      else
      {
        details.response =
            rpc_client_.CallSpecificAddress(counter_party.identifier(), RPC_BEACON_SETUP,
                                            BeaconSetupServiceProtocol::SUBMIT_SHARE, from, share);
      }
    }
  }

  // Gettting response
  bool all_delivered = true;
  // TODO(tfr): Check that all is delivered

  if (all_delivered)
  {
    return State::WAIT_FOR_SHARES;
  }

  return State::SEND_SHARES;
}

bool BeaconSetupService::SubmitShare(Identity from, PrivateKey share)
{
  std::lock_guard<std::mutex> lock(mutex_);
  // TODO(tfr): Check signature
  // TODO(tfr):  member is part of cabinet

  ShareSubmission submission;
  submission.from  = from;
  submission.share = std::move(share);

  submitted_shares_[from] = submission;

  return true;
}

BeaconSetupService::State BeaconSetupService::OnWaitForShares()
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (submitted_shares_.size() == beacon_->aeon.members.size())
    {
      return State::GENERATE_KEYS;
    }
  }

  state_machine_->Delay(std::chrono::milliseconds(10));
  return State::WAIT_FOR_SHARES;
}

BeaconSetupService::State BeaconSetupService::OnGenerateKeys()
{
  std::lock_guard<std::mutex> lock(mutex_);

  // Adding shares
  for (auto &t : submitted_shares_)
  {
    auto &s = t.second;
    // TODO: Test that it exists just to be on the sure to be sure
    auto vv = member_verification_vector_[s.from.identifier()];
    if (!beacon_->manager.AddShare(s.from, s.share, vv))
    {
      // TODO(tfr): Serializable error
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
  member_details_queue_.clear();
  member_details_.clear();
  share_delivery_details_.clear();
  submitted_shares_.clear();
  member_verification_vector_.clear();

  return State::IDLE;
}

void BeaconSetupService::QueueSetup(SharedAeonExecutionUnit beacon)
{
  std::lock_guard<std::mutex> lock(mutex_);
  assert(beacon != nullptr);

  aeon_exe_queue_.push_back(beacon);
}

// TODO(tfr): ... steps - rbc? ...
// TODO(tfr): support for complaints

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
