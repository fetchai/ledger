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
#include "crypto/sha256.hpp"
#include "dkg/dkg_service.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/subscription.hpp"

#include <chrono>
#include <cstdint>

namespace fetch {
namespace dkg {
namespace {

using namespace std::chrono_literals;

using byte_array::ConstByteArray;
using serializers::ByteArrayBuffer;

using MuddleAddress = muddle::Packet::Address;
using State         = DkgService::State;
using PromiseState  = service::PromiseState;

constexpr uint64_t READ_AHEAD     = 3;
constexpr uint64_t HISTORY_LENGTH = 10;

const ConstByteArray GENESIS_PAYLOAD = "=~=~ Genesis ~=~=";

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
  case DkgService::State::WAIT_FOR_DKG_COMPLETION:
    text = "Wait for DKG Completion";
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

bn::G2 DkgService::group_g_;

/**
 * Creates a instance of the DKG Service
 *
 * @param endpoint The muddle endpoint to communicate on
 * @param address The muddle endpoint address
 * @param dealer_address The dealer address
 */
DkgService::DkgService(Endpoint &endpoint, ConstByteArray address)
  : address_{std::move(address)}
  , endpoint_{endpoint}
  , rpc_server_{endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , rpc_client_{"dkg", endpoint_, SERVICE_DKG, CHANNEL_RPC}
  , state_machine_{std::make_shared<StateMachine>("dkg", State::BUILD_AEON_KEYS, ToString)}
  , rbc_{endpoint_, address_, current_cabinet_,
         [this](MuddleAddress const &address, ConstByteArray const &payload) -> void {
           OnRbcDeliver(address, payload);
         }}
  , dkg_{address_, current_cabinet_, current_threshold_, *this}
{
  group_g_.clear();
  group_g_ = dkg_.group();

  // RPC server registration
  rpc_proto_ = std::make_unique<DkgRpcProtocol>(*this);
  rpc_server_.Add(RPC_DKG_BEACON, rpc_proto_.get());

  // configure the state handlers
  // clang-format off
          state_machine_->RegisterHandler(State::BUILD_AEON_KEYS,     this, &DkgService::OnBuildAeonKeysState);
          state_machine_->RegisterHandler(State::WAIT_FOR_DKG_COMPLETION,  this, &DkgService::OnWaitForDkgCompletionState);
          state_machine_->RegisterHandler(State::BROADCAST_SIGNATURE, this, &DkgService::OnBroadcastSignatureState);
          state_machine_->RegisterHandler(State::COLLECT_SIGNATURES,  this, &DkgService::OnCollectSignaturesState);
          state_machine_->RegisterHandler(State::COMPLETE,            this, &DkgService::OnCompleteState);
  // clang-format on
}

/** RPC Handler: Secret share submission
 *
 * @param address The address of the shares owner
 * @param shares The pair of secret shares to be submitted
 */
void DkgService::SubmitShare(MuddleAddress const &                      address,
                             std::pair<std::string, std::string> const &shares)
{
  dkg_.OnNewShares(address, shares);
}

/**
 * DKG call to send secret share
 *
 * @param destination Muddle address of recipient
 * @param shares The pair of secret shares to be sent
 */
void DkgService::SendShares(MuddleAddress const &                      destination,
                            std::pair<std::string, std::string> const &shares)
{
  rpc_client_.CallSpecificAddress(destination, RPC_DKG_BEACON, DkgRpcProtocol::SUBMIT_SHARE,
                                  address_, shares);
}

/**
 * RPC Handler: Signature submission
 *
 * @param round The current round number
 * @param id The id of the issuer
 * @param public_key The public key for signature verification
 * @param signature The signature to be submitted
 * @param signature The signature to be submitted
 */
void DkgService::SubmitSignatureShare(uint64_t round, uint32_t const &id,
                                      std::string const &sig_str)
{
  bn::G1 signature;
  signature.clear();
  signature.setStr(sig_str);

  FETCH_LOCK(round_lock_);
  pending_signatures_.emplace_back(Submission{round, id, signature});
}

/**
 * Send a message using the reliable broadcast protocol
 *
 * @param msg Serialised message
 */
void DkgService::SendReliableBroadcast(RBCMessageType const &msg)
{
  DKGSerializer serialiser;
  msg.Serialize(serialiser);
  rbc_.SendRBroadcast(serialiser.data());
}

/**
 * Handler for messages which have completed RBC protocol
 *
 * @param from Muddle address of the sender of the message
 * @param payload Serialised message
 */
void DkgService::OnRbcDeliver(MuddleAddress const &from, byte_array::ConstByteArray const &payload)
{
  DKGEnvelop    env;
  DKGSerializer serializer{payload};
  serializer >> env;
  dkg_.OnDkgMessage(from, env.Message());
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
  dkg_.BroadcastShares();
  // since we are the dealer we do not need to request a signature from ourselves
  return State::WAIT_FOR_DKG_COMPLETION;
}

/**
 * State Handler for WAIT_FOR_DKG_COMPLETION
 *
 * @return The next state to progress to
 */
State DkgService::OnWaitForDkgCompletionState()
{
  if (!dkg_.finished())
  {
    state_machine_->Delay(5000ms);
    return State::WAIT_FOR_DKG_COMPLETION;
  }
  else
  {
    aeon_secret_share_.clear();
    aeon_public_key_.clear();
    dkg_.SetDkgOutput(aeon_public_key_, aeon_secret_share_, aeon_public_key_shares_,
                      aeon_qual_set_);
    return State::BROADCAST_SIGNATURE;
  }
}

/**
 * State Handler for BROADCAST_SIGNATURE
 *
 * @return The next state to progress to
 */
State DkgService::OnBroadcastSignatureState()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Broadcast signature");

  State next_state{State::COLLECT_SIGNATURES};

  uint64_t const this_round = current_round_.load();

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

  FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " payload: ", payload.ToBase64(),
                 " (round: ", this_round, ")");

  // create the signature
  bn::G1 signature{SignShare(payload, aeon_secret_share_)};

  // Sanity check: verify own signature

  if (!VerifySign(aeon_public_key_shares_[id_], payload, signature, group_g_))
  {
    FETCH_LOG_ERROR("Node ", id_, " computed bad share for payload ", payload.ToBase64());
    state_machine_->Delay(500ms);
    return State::BROADCAST_SIGNATURE;  // keep in a loop
  }
  else
  {
    std::string sig_str = signature.getStr();
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " computed signature ", sig_str, " for round ",
                   this_round);
    FETCH_LOCK(cabinet_lock_);
    for (auto const &member : aeon_qual_set_)
    {
      if (member == address_)
      {
        // we do not need to RPC call to ourselves we can simply provide the signature submission
        FETCH_LOCK(round_lock_);
        pending_signatures_.emplace_back(Submission{this_round, id_, signature});
      }
      else
      {
        // submit the signature to the cabinet member
        rpc_client_.CallSpecificAddress(member, RPC_DKG_BEACON, DkgRpcProtocol::SUBMIT_SIGNATURE,
                                        this_round, id_, sig_str);
      }
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
  FETCH_LOG_DEBUG(LOGGING_NAME, "Collect signatures");
  State next_state{State::COLLECT_SIGNATURES};

  FETCH_LOCK(round_lock_);

  auto const this_round = current_round_.load();

  FETCH_LOG_DEBUG(LOGGING_NAME, "OnCollectSignaturesState round: ", this_round);

  // Step 1. Process the signature submission pool

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
        if (!VerifySign(aeon_public_key_shares_[it->id], payload, it->signature, group_g_))
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "Node ", id_, " failed to verify signature from node ",
                          it->id, " for round ", it->round, ". Discarding");
        }
        else
        {
          // successful verified this signature share - add it to the round
          FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " add share from node ", it->id, " for round ",
                         it->round);
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

  // TODO(HUT): looks like a bug here
  // Step 2. Determine if we have completed any signatures
  if (!round->HasSignature() && round->GetNumShares() >= current_threshold_ + 1)
  {
    // recover the complete signature
    round->RecoverSignature();

    // verify that the signature is correct
    if (!VerifySign(aeon_public_key_, payload, round->round_signature(), group_g_))
    {
      FETCH_LOG_CRITICAL(LOGGING_NAME, "Node ", id_,
                         " failed to lookup verify signature for payload ", payload.ToBase64());
      state_machine_->Delay(500ms);
      return State::COLLECT_SIGNATURES;  // keep in a loop
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Node: ", id_, " beacon: ", round->GetRoundEntropy().ToBase64(),
                   " round: ", round->round());

    // have completed this iteration now
    ++current_round_;

    next_state = State::COMPLETE;
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Still awaiting shares. Threshold: ", current_threshold_,
                   " Number of shares: ", round->GetNumShares(),
                   " has signature: ", round->HasSignature());
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

  // clean up the resources
  {
    FETCH_LOCK(round_lock_);

    // cache the current round
    uint64_t const current_round = current_round_.load();

    // remove the elements fromt the history buffer
    if (current_round >= HISTORY_LENGTH)
    {
      rounds_.erase(rounds_.begin(), rounds_.lower_bound(current_round - HISTORY_LENGTH));
    }
  }

  state_machine_->Delay(500ms);

  return State::COMPLETE;
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
