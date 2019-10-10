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

#include "core/service_ids.hpp"
#include "ledger/consensus/consensus.hpp"
#include "ledger/protocols/notarisation_service.hpp"

#include <memory>

namespace fetch {
namespace ledger {

char const *StateToString(NotarisationService::State state);

NotarisationService::NotarisationService(MuddleInterface &muddle)
  : endpoint_{muddle.GetEndpoint()}
  , state_machine_{std::make_shared<StateMachine>("NotarisationService", State::KEY_ROTATION,
                                                  StateToString)}
  , rpc_client_{"NotarisationService", endpoint_, SERVICE_MAIN_CHAIN, CHANNEL_RPC}
  , notarisation_protocol_{*this}
{
  // Attaching the protocol
  rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_MAIN_CHAIN, CHANNEL_RPC);
  rpc_server_->Add(RPC_NOTARISATION, &notarisation_protocol_);

  // clang-format off
      state_machine_->RegisterHandler(State::KEY_ROTATION, this, &NotarisationService::OnKeyRotation);
      state_machine_->RegisterHandler(State::NOTARISATION_SYNCHRONISATION, this, &NotarisationService::OnNotarisationSynchronisation);
      state_machine_->RegisterHandler(State::COLLECT_NOTARISATIONS, this, &NotarisationService::OnCollectNotarisations);
      state_machine_->RegisterHandler(State::VERIFY_NOTARISATIONS, this, &NotarisationService::OnVerifyNotarisations);
      state_machine_->RegisterHandler(State::COMPLETE, this, &NotarisationService::OnComplete);
  // clang-format on
}

NotarisationService::State NotarisationService::OnKeyRotation()
{
  FETCH_LOCK(mutex_);

  // Checking whether the new keys have been generated
  if (!aeon_exe_queue_.empty())
  {
    active_exe_unit_ = aeon_exe_queue_.front();
    aeon_exe_queue_.pop_front();

    return State::NOTARISATION_SYNCHRONISATION;
  }

  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::KEY_ROTATION;
}

NotarisationService::State NotarisationService::OnNotarisationSynchronisation()
{
  FETCH_LOCK(mutex_);

  if (NextBlockHeight() < active_exe_unit_->aeon.round_start)
  {
    // TODO(JMW): Should go fetch notarisations
    state_machine_->Delay(std::chrono::milliseconds(500));
    return State::NOTARISATION_SYNCHRONISATION;
  }

  return State::COLLECT_NOTARISATIONS;
}

NotarisationService::State NotarisationService::OnCollectNotarisations()
{
  FETCH_LOCK(mutex_);

  // Want to obtain notarisations for next block height
  uint64_t next_height = NextBlockHeight();

  // Randomly select someone from qual to query
  auto &      qual                = active_exe_unit_->manager.qual();
  std::size_t random_member_index = static_cast<size_t>(rand()) % qual.size();
  auto        it                  = std::next(qual.begin(), long(random_member_index));

  // Try again if we picked out own address
  if (*it == endpoint_.GetAddress())
  {
    return State::COLLECT_NOTARISATIONS;
  }

  notarisation_promise_ = rpc_client_.CallSpecificAddress(
      *it, RPC_NOTARISATION, NotarisationServiceProtocol::GET_NOTARISATIONS, next_height);

  // Note: this delay is effectively how long we wait for the network event to resolve
  state_machine_->Delay(std::chrono::milliseconds(50));

  return State::VERIFY_NOTARISATIONS;
}

NotarisationService::State NotarisationService::OnVerifyNotarisations()
{
  std::unordered_map<BlockHash, NotarisationInformation> ret;

  try
  {
    // Attempt to resolve the promise and add it
    if (!notarisation_promise_->IsSuccessful() ||
        !notarisation_promise_->As<std::unordered_map<BlockHash, NotarisationInformation>>(ret))
    {
      return State::COLLECT_NOTARISATIONS;
    }
  }
  catch (...)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Promise timed out and threw! This should not happen.");
  }

  // Note: don't lock until the promise has resolved (above)! Otherwise the system can deadlock
  // due to everyone trying to lock and resolve each others' signatures
  std::unordered_set<BlockHash> can_verify;
  uint64_t                      next_height = NextBlockHeight();
  {
    FETCH_LOCK(mutex_);

    if (ret.empty())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Peer wasn't ready when asking for signatures");
      state_machine_->Delay(std::chrono::milliseconds(100));

      return State::COLLECT_NOTARISATIONS;
    }

    // Success - Add relevant info
    for (auto const &block_hash_sigs : ret)
    {
      BlockHash block_hash             = block_hash_sigs.first;
      auto &    existing_notarisations = notarisations_being_built_[next_height][block_hash];
      for (auto const &address_sig_pairs : block_hash_sigs.second)
      {
        if (existing_notarisations.find(address_sig_pairs.first) == existing_notarisations.end())
        {
          if (active_exe_unit_->manager.VerifySignatureShare(block_hash, address_sig_pairs.second,
                                                             address_sig_pairs.first))
          {
            FETCH_LOG_INFO(LOGGING_NAME, "Added notarisation from node ",
                           active_exe_unit_->manager.cabinet_index(address_sig_pairs.first));
            existing_notarisations[address_sig_pairs.first] = address_sig_pairs.second;
          }
        }
        // If we have collected enough signatures for this block hash then move onto next hash
        if (existing_notarisations.size() > active_exe_unit_->manager.polynomial_degree() + 1)
        {
          can_verify.insert(block_hash);
          break;
        }
      }
    }
  }  // Mutex unlocks here since verification can take some time

  if (!can_verify.empty())
  {
    for (auto const &hash : can_verify)
    {
      auto sig = active_exe_unit_->manager.ComputeGroupSignature(
          notarisations_being_built_[next_height][hash]);
      assert(active_exe_unit_->manager.VerifyGroupSignature(hash, sig));
      notarisations_built_[next_height][hash] = sig;
    }
    return State::COMPLETE;
  }

  return State::COLLECT_NOTARISATIONS;
}

NotarisationService::State NotarisationService::OnComplete()
{
  FETCH_LOCK(mutex_);
  uint64_t next_height = NextBlockHeight();
  for (const auto &new_notarisation : notarisations_built_[next_height])
  {
    callback_(new_notarisation.first);
  }
  highest_notarised_block_height_ = next_height;

  if (NextBlockHeight() > active_exe_unit_->aeon.round_end)
  {
    // Completed notarisation of a sequence of blocks during aeon. Any notarised blocks not received
    // through RPC will be obtained via broadcast
    return State::KEY_ROTATION;
  }
  return State::COLLECT_NOTARISATIONS;
}

NotarisationService::BlockNotarisationShares NotarisationService::GetNotarisations(
    BlockHeight const &height)
{
  FETCH_LOCK(mutex_);
  if (notarisations_being_built_.find(height) == notarisations_being_built_.end())
  {
    return {};
  }
  return notarisations_being_built_.at(height);
}

void NotarisationService::NotariseBlock(BlockBody const &block)
{
  FETCH_LOCK(mutex_);
  // Return if not eligible to notarise
  if (!active_exe_unit_ || block.block_number < active_exe_unit_->aeon.round_start ||
      block.block_number >= active_exe_unit_->aeon.round_end)
  {
    return;
  }
  else if (notarisations_built_.find(block.block_number) != notarisations_built_.end() &&
           notarisations_built_.at(block.block_number).find(block.hash) !=
               notarisations_built_.at(block.block_number).end())
  {
    // TODO(JMW): Block has already been notarised -> tell main chain
    return;
  }
  // Determine rank of miner in qual
  if (block.block_number == active_exe_unit_->aeon.round_start)
  {
    assert(block.block_entropy.qualified == active_exe_unit_->manager.qual());
  }
  auto entropy_ranked_cabinet = Consensus::QualWeightedByEntropy(
      active_exe_unit_->manager.qual(), block.block_entropy.EntropyAsU64());
  auto miner_position =
      std::find(entropy_ranked_cabinet.begin(), entropy_ranked_cabinet.end(), block.miner_id);
  assert(miner_position != entropy_ranked_cabinet.end());
  auto miner_rank =
      static_cast<uint32_t>(std::distance(entropy_ranked_cabinet.begin(), miner_position));

  // TODO(JMW): Is previous block notarised? Is block too far back behind cutoff to be notarised?

  // Check if we have previously signed a higher ranked block at the same height
  if (previous_notarisation_rank_.find(block.block_number) != previous_notarisation_rank_.end())
  {
    if (previous_notarisation_rank_.at(block.block_number) > miner_rank)
    {
      return;
    }
  }

  auto notarisation = active_exe_unit_->manager.Sign(block.hash);
  assert(active_exe_unit_->manager.VerifySignatureShare(block.hash, notarisation.signature,
                                                        notarisation.identity.identifier()));
  notarisations_being_built_[block.block_number][block.hash].insert(
      {notarisation.identity.identifier(), notarisation.signature});
  previous_notarisation_rank_[block.block_number] = miner_rank;
}

std::vector<std::weak_ptr<core::Runnable>> NotarisationService::GetWeakRunnables()
{
  return {state_machine_};
}

void NotarisationService::NewAeonExeUnit(SharedAeonExecutionUnit const &keys)
{
  aeon_exe_queue_.push_back(keys);
}

uint64_t NotarisationService::NextBlockHeight()
{
  if (highest_notarised_block_height_ == UINT64_MAX)
  {
    return 0;
  }
  return highest_notarised_block_height_ + 1;
}

void NotarisationService::SetNotarisedBlockCallback(CallbackFunction callback)
{
  FETCH_LOCK(mutex_);
  callback_ = std::move(callback);
}

char const *StateToString(NotarisationService::State state)
{
  char const *text = "unknown";

  switch (state)
  {
  case NotarisationService::State::KEY_ROTATION:
    text = "Waiting for setup completion";
    break;
  case NotarisationService::State::NOTARISATION_SYNCHRONISATION:
    text = "Preparing entropy generation";
    break;
  case NotarisationService::State::COLLECT_NOTARISATIONS:
    text = "Collecting signatures";
    break;
  case NotarisationService::State::VERIFY_NOTARISATIONS:
    text = "Verifying signatures";
    break;
  case NotarisationService::State::COMPLETE:
    text = "Completion state";
    break;
  }

  return text;
}
}  // namespace ledger
}  // namespace fetch
