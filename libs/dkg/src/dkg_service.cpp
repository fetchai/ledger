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
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/service_ids.hpp"
#include "crypto/bls_dkg.hpp"
#include "crypto/sha256.hpp"
#include "dkg/dkg_service.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/subscription.hpp"

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
constexpr uint64_t    READ_AHEAD   = 3;

const ConstByteArray GENESIS_PAYLOAD = "=~=~ Genesis ~=~=";

/**
 * Creates a BLS ID from a specified Muddle Address
 *
 * @param address The input muddle address
 * @return The generated BLS ID
 */
crypto::bls::Id CreateIdFromAddress(ConstByteArray const &address)
{
  auto const seed = crypto::bls::HashToPrivateKey(address);
  return crypto::bls::Id{seed.v};
}

/**
 * Converts the state enum to a string
 *
 * @param state The state to convert
 * @return The string representation for the state
 */
char const *ToString(DkgService::State state)
{
  char const *text = "unknown";

  switch (state)
  {
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

}  // namespace

/**
 * Creates a instance of the DKG Service
 *
 * @param endpoint The muddle endpoint to communicate on
 * @param address The muddle endpoint address
 * @param dealer_address The dealer address
 */
DkgService::DkgService(Endpoint &endpoint, ConstByteArray address, ConstByteArray dealer_address)
  : address_{std::move(address)}
  , id_{CreateIdFromAddress(address_)}
  , dealer_address_{std::move(dealer_address)}
  , is_dealer_{address_ == dealer_address_}
  , endpoint_{endpoint}
  , rpc_server_{endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , rpc_client_{"dkg", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , state_machine_{std::make_shared<StateMachine>("dkg", State::BUILD_AEON_KEYS, ToString)}
{
  // RPC server registration
  rpc_proto_ = std::make_unique<DkgRpcProtocol>(*this);
  rpc_server_.Add(RPC_DKG_BEACON, rpc_proto_.get());

  // configure the state handlers
  // clang-format off
  state_machine_->RegisterHandler(State::BUILD_AEON_KEYS,     this, &DkgService::OnBuildAeonKeysState);
  state_machine_->RegisterHandler(State::REQUEST_SECRET_KEY,  this, &DkgService::OnRequestSecretKeyState);
  state_machine_->RegisterHandler(State::WAIT_FOR_SECRET_KEY, this, &DkgService::OnWaitForSecretKeyState);
  state_machine_->RegisterHandler(State::BROADCAST_SIGNATURE, this, &DkgService::OnBroadcastSignatureState);
  state_machine_->RegisterHandler(State::COLLECT_SIGNATURES,  this, &DkgService::OnCollectSignaturesState);
  state_machine_->RegisterHandler(State::COMPLETE,            this, &DkgService::OnCompleteState);
  // clang-format on
}

/**
 * RPC Handler: Handler for client secret request
 *
 * @param address The muddle address of the requester
 * @return The response structure
 */
DkgService::SecretKeyReq DkgService::RequestSecretKey(MuddleAddress const &address)
{
  SecretKeyReq req{};

  FETCH_LOCK(cabinet_lock_);
  FETCH_LOCK(dealer_lock_);

  auto it = current_cabinet_secrets_.find(address);
  if (it != current_cabinet_secrets_.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", address.ToBase64(), " requested secret share");

    // populate the request
    req.success           = true;
    req.secret_share      = it->second;
    req.shared_public_key = shared_public_key_;
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to provide node: ", address.ToBase64(),
                   " with secret share");
  }

  return req;
}

/**
 * RPC Handler: Signature submission
 *
 * @param round The current round number
 * @param id The id of the issuer
 * @param public_key The public key for signature verification
 * @param signature The signature to be submitted
 */
void DkgService::SubmitSignatureShare(uint64_t round, crypto::bls::Id const &id,
                                      crypto::bls::PublicKey const &public_key,
                                      crypto::bls::Signature const &signature)
{
  FETCH_LOG_TRACE(LOGGING_NAME, "Submit of signature for round ", round);

  FETCH_LOCK(round_lock_);
  pending_signatures_.emplace_back(Submission{round, id, public_key, signature});
}

/**
 * Generate entropy for a given block, identified by digest and block number
 *
 * @param block_digest The block digest
 * @param block_number The block number
 * @param entropy The output entropy value
 * @return The associated status result for the operation
 */
DkgService::Status DkgService::GenerateEntropy(Digest block_digest, uint64_t block_number,
                                               uint64_t &entropy)
{
  FETCH_UNUSED(block_digest);

  Status status{Status::NOT_READY};

  // lookup the round
  auto const round = LookupRound(block_number);
  if (round && round->HasSignature())
  {
    entropy = round->GetEntropy();
    status  = Status::OK;

    // signal that the round has been consumed
    ++earliest_completed_round_;
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "Trying to generate entropy ahead in time! block_number: ", block_number);
  }

  return status;
}

/**
 * State Handler for BUILD_AEON_KEYS
 *
 * @return The next state to progress to
 */
State DkgService::OnBuildAeonKeysState()
{
  if (is_dealer_)
  {
    BuildAeonKeys();

    // since we are the dealer we do not need to request a signature from ourselves
    return State::BROADCAST_SIGNATURE;
  }

  return State::REQUEST_SECRET_KEY;
}

/**
 * State Handler for REQUEST_SECRET_KEY
 *
 * @return The next state to progress to
 */
State DkgService::OnRequestSecretKeyState()
{
  // request from the beacon for the secret key
  pending_promise_ = rpc_client_.CallSpecificAddress(dealer_address_, RPC_DKG_BEACON,
                                                     DkgRpcProtocol::REQUEST_SECRET, address_);

  return State::WAIT_FOR_SECRET_KEY;
}

/**
 * State Handler for WAIT_FOR_SECRET_KEY
 *
 * @return The next state to progress to
 */
State DkgService::OnWaitForSecretKeyState()
{
  State next_state{State::WAIT_FOR_SECRET_KEY};

  FETCH_LOG_DEBUG(LOGGING_NAME, "State: Wait for secret");

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
        next_state             = State::BROADCAST_SIGNATURE;
        waiting                = false;
        aeon_secret_share_     = response.secret_share;
        aeon_share_public_key_ = crypto::bls::GetPublicKey(aeon_secret_share_);
        aeon_public_key_       = response.shared_public_key;
        pending_promise_.reset();
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Response unsuccessful, retrying");
        next_state = State::REQUEST_SECRET_KEY;
      }

      break;
    }
    case PromiseState::FAILED:
    case PromiseState::TIMEDOUT:
      FETCH_LOG_INFO(LOGGING_NAME, "Response timed out, retrying");
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

/**
 * State Handler for BROADCAST_SIGNATURE
 *
 * @return The next state to progress to
 */
State DkgService::OnBroadcastSignatureState()
{
  State next_state{State::COLLECT_SIGNATURES};

  auto const this_round = current_round_.load();

  FETCH_LOG_DEBUG(LOGGING_NAME, "OnBroadcastSignatureState round: ", this_round);

  // lookup / determine the payload we are expecting with the message
  ConstByteArray payload{};
  if (this_round == 0)
  {
    payload = GENESIS_PAYLOAD;
  }
  else
  {
    if (!GetSignaturePayload(this_round - 1u, payload))
    {
      FETCH_LOG_CRITICAL(LOGGING_NAME,
                         "Failed to lookup previous rounds signature data for broadcast");
      state_machine_->Delay(500ms);
      return State::BROADCAST_SIGNATURE;  // keep in a loop
    }
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Payload: ", payload.ToBase64(), " (round: ", this_round, ")");

  // create the signature
  auto signature = crypto::bls::Sign(aeon_secret_share_, payload);

  FETCH_LOCK(cabinet_lock_);
  for (auto const &member : current_cabinet_)
  {
    if (member == address_)
    {
      // we do not need to RPC call to ourselves we can simply provide the signature submission
      FETCH_LOCK(round_lock_);
      pending_signatures_.emplace_back(
          Submission{this_round, id_, aeon_share_public_key_, signature});
    }
    else
    {
      FETCH_LOG_TRACE(LOGGING_NAME, "Submitting Signature to: ", member.ToBase64());

      // submit the signature to the cabinet member
      rpc_client_.CallSpecificAddress(member, RPC_DKG_BEACON, DkgRpcProtocol::SUBMIT_SIGNATURE,
                                      current_round_.load(), id_, aeon_share_public_key_,
                                      signature);
    }
  }

  return next_state;
}

/**
 * State Handler for COLLECT_SIGNATURES
 *
 * @return The next state to progress to
 */
State DkgService::OnCollectSignaturesState()
{
  State next_state{State::COLLECT_SIGNATURES};

  FETCH_LOCK(round_lock_);

  auto const this_round = current_round_.load();

  FETCH_LOG_DEBUG(LOGGING_NAME, "OnCollectSignaturesState round: ", this_round);

  // Step 1. Process the signature submission pool

  // lookup / determine the payload we are expecting with the message
  ConstByteArray payload;
  if (this_round == 0)
  {
    payload = GENESIS_PAYLOAD;
  }
  else
  {
    if (!GetSignaturePayload(this_round - 1u, payload))
    {
      FETCH_LOG_CRITICAL(LOGGING_NAME, "Failed to lookup previous rounds signature data");
      state_machine_->Delay(500ms);
      return State::COLLECT_SIGNATURES;  // keep in a loop
    }
  }

  // lookup the requesting round
  auto const round = LookupRound(this_round, true);
  assert(static_cast<bool>(round));

  bool updates{false};

  // process the pending signature submissions
  auto it = pending_signatures_.begin();
  while (it != pending_signatures_.end())
  {
    if (it->round <= this_round)
    {
      if (it->round == this_round)
      {
        // verify the signature
        if (!crypto::bls::Verify(it->signature, it->public_key, payload))
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "Failed to very signature submision. Discarding");
        }
        else
        {
          // successful verified this signature share - add it to the round
          round->AddShare(it->id, it->signature);
          updates = true;
        }
      }

      // this message has expired so we no longer
      it = pending_signatures_.erase(it);
    }
    else
    {
      // move on to the next one
      ++it;
    }
  }

  // Step 2. Determine if we have completed any signatures
  if (!round->HasSignature() && round->GetNumShares() >= current_threshold_)
  {
    // recover the complete signature
    round->RecoverSignature();

    // verify that the signature is correct
    if (!crypto::bls::Verify(round->round_signature(), aeon_public_key_, payload))
    {
      FETCH_LOG_CRITICAL(LOGGING_NAME, "Failed to lookup verify signature");
      state_machine_->Delay(500ms);
      return State::COLLECT_SIGNATURES;  // keep in a loop
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Beacon: ", round->GetRoundEntropy().ToBase64(),
                   " round: ", round->round());

    // have completed this iteration now
    ++current_round_;

    next_state = State::COMPLETE;
  }

  // if there have been no updates on this iteration, wait for a period of time
  if (!updates)
  {
    state_machine_->Delay(500ms);
  }

  return next_state;
}

/**
 * State Handler for COMPLETE
 *
 * The complete state is used as an idling state for the FSM
 *
 * @return The next state to progress to
 */
State DkgService::OnCompleteState()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "State: Complete round: ", requesting_iteration_.load(),
                  " read: ", current_iteration_.load());

  // calculate the current number of signatures ahead
  if (current_round_ <= earliest_completed_round_)
  {
    return State::BROADCAST_SIGNATURE;
  }
  else
  {
    uint64_t const delta = current_round_ - earliest_completed_round_;
    if (delta < READ_AHEAD)
    {
      return State::BROADCAST_SIGNATURE;
    }
  }

  // TODO(issue 1285): Improve resource usage here for round cache
  state_machine_->Delay(500ms);

  return State::COMPLETE;
}

/**
 * Builds a new set of keys for the aeon
 *
 * @return true if successful, otherwise false
 */
bool DkgService::BuildAeonKeys()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Build new aeons key shares");

  FETCH_LOCK(cabinet_lock_);
  FETCH_LOCK(dealer_lock_);

  // generate the master key
  crypto::bls::PrivateKeyList master_key(current_threshold_);
  for (auto &key : master_key)
  {
    key = crypto::bls::PrivateKeyByCSPRNG();
  }

  // generate the corresponding shared public key
  shared_public_key_ = crypto::bls::GetPublicKey(master_key[0]);

  // build a secret share for each of the
  current_cabinet_secrets_.clear();
  for (auto const &member : current_cabinet_)
  {
    // use the unique muddle address to form the id
    auto const id = CreateIdFromAddress(member);

    // create the secret share based on the member id
    auto const secret_share = crypto::bls::PrivateKeyShare(master_key, id);

    if (member == address_)
    {
      aeon_secret_share_     = secret_share;
      aeon_share_public_key_ = crypto::bls::GetPublicKey(aeon_secret_share_);
      aeon_public_key_       = shared_public_key_;
    }

    // update the cabinet map
    current_cabinet_secrets_[member] = secret_share;
  }

  return true;
}

/**
 * Gets the payload to be signed for a specified round interval
 *
 * @param round The round being queried
 * @param payload The output payload to be populated
 * @return true if successful, otherwise false
 */
bool DkgService::GetSignaturePayload(uint64_t round, ConstByteArray &payload)
{
  bool success{false};

  auto const round_info = LookupRound(round);
  if (round_info && round_info->HasSignature())
  {
    payload = round_info->GetRoundEntropy();
    success = true;
  }

  return success;
}

/**
 * Lookup the round information for a specified round index
 *
 * @param round The round index being requested
 * @param create Flag to signal if a round should be created if it doesn't exist
 * @return If successful the requested round object, otherwise a nullptr
 */
RoundPtr DkgService::LookupRound(uint64_t round, bool create)
{
  RoundPtr round_ptr{};
  FETCH_LOCK(round_lock_);

  auto it = rounds_.find(round);
  if (it == rounds_.end())
  {
    if (create)
    {
      // create the new round
      round_ptr = std::make_shared<Round>(round);

      // store it in the map
      rounds_[round] = round_ptr;
    }
  }
  else
  {
    round_ptr = it->second;
  }

  return round_ptr;
}

}  // namespace dkg
}  // namespace fetch
