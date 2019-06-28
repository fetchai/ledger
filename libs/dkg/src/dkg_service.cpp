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

#include "core/logging.hpp"
#include "core/service_ids.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/byte_array.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/subscription.hpp"
#include "network/muddle/packet.hpp"
#include "dkg/dkg_service.hpp"
#include "dkg/dkg_messages.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/sha256.hpp"

#include <chrono>

namespace fetch {
namespace dkg {
namespace {

using namespace std::chrono_literals;

using byte_array::ConstByteArray;
using serializers::ByteArrayBuffer;

using MuddleAddress = muddle::Packet::Address;
using State         = DkgService::State;
using PromiseState  = service::PromiseState;

constexpr char const *LOGGING_NAME = "DkgService";

template <typename T>
bool ParseMessage(ConstByteArray const &payload, T &message)
{
  bool success{false};

  try
  {
    ByteArrayBuffer buffer{payload};
    buffer >> message;

    success = true;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse network message: ", ex.what());
  }

  return success;
}

crypto::bls::Id CreateIdFromAddress(ConstByteArray const &address)
{
  auto const seed = crypto::bls::HashToPrivateKey(address);
  return crypto::bls::Id{seed.v};
}

char const *ToString(DkgService::State state)
{
  char const *text = "unknown";

  switch (state)
  {
  case DkgService::State::REGISTER:
    text = "Register";
    break;
  case DkgService::State::WAIT_FOR_REGISTRATION:
    text = "Wait for Registration";
    break;
  case DkgService::State::BUILD_AEON_KEYS:
    text = "Build Aeon Keys";
    break;
  case DkgService::State::REQUEST_SECRET_KEY:
    text = "Request Secret Key";
    break;
  case DkgService::State::WAIT_FOR_SECRET_KEY:
    text = "Wait for Secret Key";
    break;
  case DkgService::State::BROADCAST_SIGNATURE:
    text = "Broadcast Signature";
    break;
  case DkgService::State::COLLECT_SIGNATURES:
    text = "Collect Signatures";
    break;
  case DkgService::State::COMPLETE:
    text = "Complete";
    break;
  }

  return text;
}

} // namespace

DkgService::DkgService(Endpoint &endpoint, ConstByteArray address, ConstByteArray beacon_address,
                       std::size_t key_lifetime)
  : address_{std::move(address)}
  , id_{CreateIdFromAddress(address_)}
  , beacon_address_{std::move(beacon_address)}
  , is_dealer_{address_ == beacon_address_}
  , endpoint_{endpoint}
  , rpc_server_{endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , rpc_client_{"dkg", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , state_machine_{std::make_shared<StateMachine>("dkg", State::REGISTER, ToString)}
//  , contribution_subscription_{endpoint_.Subscribe(SERVICE_DKG, CHANNEL_CONTRIBUTIONS)}
//  , secret_key_subscription_{endpoint_.Subscribe(SERVICE_DKG, CHANNEL_SECRET_KEY)}
{
  // RPC server registration
  rpc_proto_ = std::make_unique<DkgRpcProtocol>(*this);
  rpc_server_.Add(RPC_DKG_BEACON, rpc_proto_.get());

  // configure the state handlers
  state_machine_->RegisterHandler(State::REGISTER, this, &DkgService::OnRegisterState);
  state_machine_->RegisterHandler(State::WAIT_FOR_REGISTRATION, this, &DkgService::OnWaitForRegistrationState);
  state_machine_->RegisterHandler(State::BUILD_AEON_KEYS, this, &DkgService::OnBuildAeonKeysState);
  state_machine_->RegisterHandler(State::REQUEST_SECRET_KEY, this, &DkgService::OnRequestSecretKeyState);
  state_machine_->RegisterHandler(State::WAIT_FOR_SECRET_KEY, this, &DkgService::OnWaitForSecretKeyState);
  state_machine_->RegisterHandler(State::BROADCAST_SIGNATURE, this, &DkgService::OnBroadcastSignatureState);
  state_machine_->RegisterHandler(State::COLLECT_SIGNATURES, this, &DkgService::OnCollectSignaturesState);
  state_machine_->RegisterHandler(State::COMPLETE, this, &DkgService::OnCompleteState);

  // TODO(EJF): Remove
  state_machine_->OnStateChange([](State current, State previous) {
    FETCH_LOG_WARN(LOGGING_NAME, "State changed to: ", ToString(current), " was: ", ToString(previous));
  });

//  // setup all the message handlers
//  contribution_subscription_->SetMessageHandler(
//      [this](ConstByteArray const &from, uint16_t service, uint16_t channel, uint16_t counter,
//             ConstByteArray const &payload, ConstByteArray const &transmitter) {
//        FETCH_UNUSED(service);
//        FETCH_UNUSED(channel);
//        FETCH_UNUSED(counter);
//        FETCH_UNUSED(transmitter);
//
//        ContributionMsg msg{};
//        if (ParseMessage(payload, msg))
//        {
//          OnContribution(msg, from);
//        }
//      });
//
//  secret_key_subscription_->SetMessageHandler(
//      [this](ConstByteArray const &from, uint16_t service, uint16_t channel, uint16_t counter,
//             ConstByteArray const &payload, ConstByteArray const &transmitter) {
//        FETCH_UNUSED(service);
//        FETCH_UNUSED(channel);
//        FETCH_UNUSED(counter);
//        FETCH_UNUSED(transmitter);
//
//        SecretKeyMsg msg{};
//        if (ParseMessage(payload, msg))
//        {
//          OnSecretKey(msg, from);
//        }
//      });

  // consistency
  if (current_cabinet_.find(address_) != current_cabinet_.end())
  {
    current_cabinet_ids_[address_] = id_; // register ourselves
  }

  FETCH_LOG_INFO(LOGGING_NAME, "#LivingTheDream...");
}

bool DkgService::RegisterCabinetMember(MuddleAddress const &address, crypto::bls::Id const &id)
{
  bool success{false};

  FETCH_LOG_WARN(LOGGING_NAME, "Attempting to register that cabinet member: ", address.ToBase64());

  FETCH_LOCK(cabinet_lock_);
  if (current_cabinet_.find(address) != current_cabinet_.end())
  {
    // create or update the id
    current_cabinet_ids_[address] = id;
  }

  if (CanBuildAeonKeys())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Can now build aeon keys");
  }

  return success;
}

DkgService::SecretKeyReq DkgService::RequestSecretKey(MuddleAddress const &address)
{
  SecretKeyReq req{};

  FETCH_LOG_WARN(LOGGING_NAME, "Requesting secret information");

  FETCH_LOCK(cabinet_lock_);
  auto it = current_cabinet_secrets_.find(address);
  if (it != current_cabinet_secrets_.end())
  {
    // populate the request
    req.success     = true;
    req.private_key = it->second;
    req.public_keys = current_cabinet_public_keys_;
  }

  return req;
}

void DkgService::SubmitSignatureShare(crypto::bls::Id const &       id,
                                      crypto::bls::PublicKey const &public_key,
                                      crypto::bls::Signature const &signature)
{
  FETCH_LOG_WARN(LOGGING_NAME, "Submit of signature");

  if (crypto::bls::Verify(signature, public_key, "foo bar baz"))
  {
    FETCH_LOCK(sig_lock_);
    sig_ids_.push_back(id);
    sig_shares_.push_back(signature);
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Recv. Invalid signature share");
  }
}

void DkgService::OnNewBlock(uint64_t block_index)
{
  FETCH_LOG_INFO(LOGGING_NAME, "On new block ", block_index);
}

DkgService::Status DkgService::GenerateEntropy(Digest block_digest, uint64_t block_number, uint64_t &entropy)
{
  FETCH_LOCK(sig_lock_);

  if (!aeon_signature_)
  {
     return Status::NOT_READY;
  }

  auto const *raw_entropy = reinterpret_cast<uint64_t const *>(aeon_signature_.get());
  entropy = *raw_entropy;
  aeon_signature_.reset();

  return Status::OK;
}

State DkgService::OnRegisterState()
{
  State next_state{State::WAIT_FOR_REGISTRATION};

  FETCH_LOG_WARN(LOGGING_NAME, "State: Register");

  if (is_dealer_)
  {
    FETCH_LOCK(cabinet_lock_);

    // consistency
    if (current_cabinet_.find(address_) != current_cabinet_.end())
    {
      current_cabinet_ids_[address_] = id_;  // register ourselves
    }

    // no registration needed, skip to the complete stage
    next_state = State::BUILD_AEON_KEYS;
  }
  else
  {
    // register with the beacon
    pending_promise_ = rpc_client_.CallSpecificAddress(beacon_address_, RPC_DKG_BEACON,
                                                       DkgRpcProtocol::REGISTER, address_, id_);
  }

  return next_state;
}

State DkgService::OnWaitForRegistrationState()
{
  State next_state{State::WAIT_FOR_REGISTRATION};

  FETCH_LOG_WARN(LOGGING_NAME, "State: Wait Register");

  bool waiting{true};

  if (pending_promise_)
  {
    auto const current_state = pending_promise_->state();

    switch (current_state)
    {
    case PromiseState::WAITING:
      break;
    case PromiseState::SUCCESS:
      next_state = State::BUILD_AEON_KEYS;
      waiting    = false;
      pending_promise_.reset();
      break;
    case PromiseState::FAILED:
    case PromiseState::TIMEDOUT:
      next_state = State::REGISTER;
      break;
    }
  }

  if (waiting)
  {
    state_machine_->Delay(500ms);
  }

  return next_state;
}

State DkgService::OnBuildAeonKeysState()
{
  State next_state{State::REQUEST_SECRET_KEY};

  if (is_dealer_)
  {
    if (CanBuildAeonKeys())
    {
      BuildAeonKeys();
    }
    else
    {
      // wait for all the registrations to complete
      next_state = State::BUILD_AEON_KEYS;
      state_machine_->Delay(1000ms);
    }
  }

  return next_state;
}

State DkgService::OnRequestSecretKeyState()
{
  // request from the beacon for the secret key
  pending_promise_ = rpc_client_.CallSpecificAddress(beacon_address_, RPC_DKG_BEACON,
                                                     DkgRpcProtocol::REQUEST_SECRET, address_);

  return State::WAIT_FOR_SECRET_KEY;
}

State DkgService::OnWaitForSecretKeyState()
{
  State next_state{State::WAIT_FOR_SECRET_KEY};

  FETCH_LOG_WARN(LOGGING_NAME, "State: Wait Secret");

  bool waiting{true};

  if (pending_promise_)
  {
    auto const current_state = pending_promise_->state();

    switch (current_state)
    {
    case PromiseState::WAITING:
      break;
    case PromiseState::SUCCESS:
    {
      auto const response = pending_promise_->As<SecretKeyReq>();

      if (response.success)
      {
        next_state       = State::BROADCAST_SIGNATURE;
        waiting          = false;
        sig_private_key_ = response.private_key;
        pending_promise_.reset();
      }
      else
      {
        next_state = State::REQUEST_SECRET_KEY;
      }

      break;
    }
    case PromiseState::FAILED:
    case PromiseState::TIMEDOUT:
      next_state = State::REQUEST_SECRET_KEY;
      break;
    }
  }

  if (waiting)
  {
    state_machine_->Delay(500ms);
  }

  return next_state;
}

State DkgService::OnBroadcastSignatureState()
{
  State next_state{State::COLLECT_SIGNATURES};

  auto signature   = crypto::bls::Sign(sig_private_key_, "foo bar baz");
  auto public_key  = crypto::bls::PublicKeyFromPrivate(sig_private_key_);

  // TODO(EJF): Thread safety when this is dynamic
  for (auto const &member : current_cabinet_)
  {
    // we do not need to send it to our selves
    if (member == address_)
    {
      continue;
    }

    // TODO(EJF): multiple promises
    // request from the beacon for the secret key
    pending_promise_ = rpc_client_.CallSpecificAddress(member, RPC_DKG_BEACON,
                                                       DkgRpcProtocol::SUBMIT_SIGNATURE, id_,
                                                       public_key, signature);
  }

  return next_state;
}

State DkgService::OnCollectSignaturesState()
{
  State next_state{State::COLLECT_SIGNATURES};

  FETCH_LOG_INFO(LOGGING_NAME, "Waiting for signatures");

  {
    FETCH_LOCK(sig_lock_);

    if (sig_shares_.size() >= current_threshold_)
    {
      // And finally we test the signature
      aeon_signature_ = std::make_unique<crypto::bls::Signature>(crypto::bls::RecoverSignature(sig_shares_, sig_ids_));

      auto const *raw = reinterpret_cast<uint8_t const *>(aeon_signature_.get());
      ConstByteArray sig_value{raw, sizeof(crypto::bls::Signature)};

      FETCH_LOG_ERROR(LOGGING_NAME, "Generated Signature: ", sig_value.ToBase64());

      // TODO(EJF): This signature needs to be verified
//      if (crypto::bls::Verify(*aeon_signature_, groups_pk, "foo bar baz"))
//      {
//        FETCH_LOG_ERROR(LOGGING_NAME, "GREATE SUCCESSE !!!!");
//      }

      next_state = State::COMPLETE;
    }
    else
    {
      state_machine_->Delay(500ms);
    }
  }

  return next_state;
}

State DkgService::OnCompleteState()
{
  FETCH_LOG_INFO(LOGGING_NAME, "State: Complete");
  state_machine_->Delay(5000ms);
  return State::COMPLETE;
}

bool DkgService::CanBuildAeonKeys() const
{
  FETCH_LOCK(cabinet_lock_);

  for (auto const &address : current_cabinet_)
  {
    if (current_cabinet_ids_.find(address) == current_cabinet_ids_.end())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Waiting on: ", address.ToBase64());
      return false;
    }
  }

  return true;
}

bool DkgService::BuildAeonKeys()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Building keys for the start of the aeon");
  FETCH_LOCK(cabinet_lock_);

  current_cabinet_public_keys_.clear();
  auto global_sk = crypto::bls::PrivateKeyByCSPRNG();
  global_pk_ = crypto::bls::PublicKeyFromPrivate(global_sk);

  crypto::bls::PrivateKeyList private_keys;
  crypto::bls::IdList ids; 


  for (std::size_t i = 0; i < current_threshold_; ++i)
  {
    auto sk = crypto::bls::PrivateKeyByCSPRNG();
    private_keys.push_back(sk);

    current_cabinet_public_keys_.emplace_back(crypto::bls::PublicKeyFromPrivate(sk));
  }

  for (auto const &address : current_cabinet_)
  {
    auto const it = current_cabinet_ids_.find(address);
    if (it == current_cabinet_ids_.end())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Unable to lookup Id for: ", address.ToBase64());
      return false;
    }

    // generate the cur
    current_cabinet_secrets_[address] = crypto::bls::PrivateKeyShare(private_keys, it->second);
  }

  return true;
}

} // namespace dkg
} // namespace fetch
