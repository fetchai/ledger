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
#include "ledger/chain/main_chain.hpp"
#include "ledger/protocols/notarisation_service.hpp"

#include <memory>

namespace fetch {
namespace ledger {

namespace {

template <typename T>
void TrimToSize(T &container, std::size_t max_size)
{
  auto it = container.begin();
  while ((it != container.end()) && (container.size() > max_size))
  {
    it = container.erase(it);
  }
}
}  // namespace

char const *StateToString(NotarisationService::State state);

NotarisationService::NotarisationService(MuddleInterface &muddle, CertificatePtr certificate,
                                         BeaconSetupService &beacon_setup)
  : endpoint_{muddle.GetEndpoint()}
  , rpc_client_{"NotarisationService", endpoint_, SERVICE_MAIN_CHAIN, CHANNEL_RPC}
  , notarisation_protocol_{*this}
  , certificate_{std::move(certificate)}
  , state_machine_{
        std::make_shared<StateMachine>("NotarisationService", State::KEY_ROTATION, StateToString)}
{
  // Attaching notarisation ready callback handler
  beacon_setup.SetNotarisationCallback([this](SharedAeonNotarisationUnit notarisation_manager) {
    FETCH_LOCK(mutex_);
    assert(notarisation_manager);
    aeon_notarisation_queue_.push_back(notarisation_manager);
    new_keys = true;
  });

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

  // If next block to be notarised is in the next aeon then reset the active notarisation unit
  if (active_notarisation_unit_ &&
      notarisation_collection_height_ > active_notarisation_unit_->round_end())
  {
    active_notarisation_unit_.reset();
  }

  // If the cutoff for blocks we notarise is greater than the end of the
  // previous aeon then reset previous
  if (previous_notarisation_unit_ && BlockNumberCutoff() > previous_notarisation_unit_->round_end())
  {
    previous_notarisation_unit_.reset();
  }

  // Checking whether the new keys have been generated
  if (!aeon_notarisation_queue_.empty())
  {
    new_keys = false;
    // Save previous as we might still need to process blocks from previous aeon
    previous_notarisation_unit_ = std::move(active_notarisation_unit_);
    active_notarisation_unit_   = aeon_notarisation_queue_.front();
    aeon_notarisation_queue_.pop_front();

    return State::NOTARISATION_SYNCHRONISATION;
  }

  state_machine_->Delay(std::chrono::milliseconds(500));
  return State::KEY_ROTATION;
}

NotarisationService::State NotarisationService::OnNotarisationSynchronisation()
{
  FETCH_LOCK(mutex_);

  // Need to collect notarisation starting from block number = round_start - 1 in order
  // to be able to include the notarisation of the previous block into the first block of this
  // aeon
  if (notarised_chain_height_ + 1 < active_notarisation_unit_->round_start() - 1)
  {
    state_machine_->Delay(std::chrono::milliseconds(500));
    return State::NOTARISATION_SYNCHRONISATION;
  }

  notarisation_collection_height_ = notarised_chain_height_ + 1;
  return State::COLLECT_NOTARISATIONS;
}

NotarisationService::State NotarisationService::OnCollectNotarisations()
{
  FETCH_LOCK(mutex_);

  // Select correct committee to collect from
  SharedAeonNotarisationUnit notarisation_unit = active_notarisation_unit_;
  if (previous_notarisation_unit_ &&
      notarisation_collection_height_ <= previous_notarisation_unit_->round_end())
  {
    assert(notarisation_collection_height_ >= previous_notarisation_unit_->round_start());
    notarisation_unit = previous_notarisation_unit_;
  }

  // Randomly select someone from qual to query
  auto        members             = notarisation_unit->notarisation_members();
  std::size_t random_member_index = static_cast<size_t>(rand()) % members.size();
  auto        it                  = std::next(members.begin(), long(random_member_index));

  // Try again if we picked out own address
  if (*it == endpoint_.GetAddress())
  {
    return State::COLLECT_NOTARISATIONS;
  }

  notarisation_promise_ = rpc_client_.CallSpecificAddress(
      *it, RPC_NOTARISATION, NotarisationServiceProtocol::GET_NOTARISATIONS,
      notarisation_collection_height_);

  // Note: this delay is effectively how long we wait for the network event to resolve
  state_machine_->Delay(std::chrono::milliseconds(50));

  return State::VERIFY_NOTARISATIONS;
}

NotarisationService::State NotarisationService::OnVerifyNotarisations()
{
  BlockNotarisationShares ret;

  try
  {
    // Attempt to resolve the promise and add it
    if (!notarisation_promise_->IsSuccessful() ||
        !notarisation_promise_->As<BlockNotarisationShares>(ret))
    {
      return State::COMPLETE;
    }
  }
  catch (...)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Promise timed out and threw! This should not happen.");
  }

  // Note: don't lock until the promise has resolved (above)! Otherwise the system can deadlock
  // due to everyone trying to lock and resolve each others' notarisations
  std::unordered_set<BlockHash> can_verify;
  SharedAeonNotarisationUnit    notarisation_unit;
  {
    FETCH_LOCK(mutex_);

    if (ret.empty())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Peer wasn't ready when asking for notarisations");
      state_machine_->Delay(std::chrono::milliseconds(100));

      return State::COMPLETE;
    }

    // Success - Add relevant info
    for (auto const &block_hash_sigs : ret)
    {
      BlockHash block_hash = block_hash_sigs.first;
      // If we have already built a notarisation for this hash then continue
      if (notarisations_built_[notarisation_collection_height_].find(block_hash) !=
          notarisations_built_[notarisation_collection_height_].end())
      {
        continue;
      }
      auto &existing_notarisations =
          notarisations_being_built_[notarisation_collection_height_][block_hash];

      // Select correct committee to verify with
      notarisation_unit = active_notarisation_unit_;
      if (previous_notarisation_unit_ &&
          notarisation_collection_height_ <= previous_notarisation_unit_->round_end())
      {
        assert(notarisation_collection_height_ >= previous_notarisation_unit_->round_start());
        notarisation_unit = previous_notarisation_unit_;
      }

      // Add signature shares for this particular block hash
      for (auto const &address_sig_pairs : block_hash_sigs.second)
      {
        // Verify and add signature if we do not have an existing signature from this qual member
        if (existing_notarisations.find(address_sig_pairs.first) == existing_notarisations.end())
        {
          SignedNotarisation signed_not = address_sig_pairs.second;
          // Verify ecdsa signature
          if (crypto::Verify(
                  address_sig_pairs.first,
                  (serializers::MsgPackSerializer() << block_hash << signed_not.notarisation_share)
                      .data(),
                  signed_not.ecdsa_signature))
          {
            // Verify notarisation
            if (notarisation_unit->Verify(block_hash, signed_not.notarisation_share,
                                          address_sig_pairs.first))
            {
              FETCH_LOG_DEBUG(LOGGING_NAME, "Added notarisation from node ",
                              notarisation_unit->Index(address_sig_pairs.first));
              existing_notarisations[address_sig_pairs.first] = address_sig_pairs.second;
            }
          }
        }
        // If we have collected enough notarisations for this block hash then move onto next hash
        if (existing_notarisations.size() == notarisation_unit->threshold())
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
      // Compute and verify group signature
      std::unordered_map<MuddleAddress, Signature> notarisation_shares;
      for (auto const &shares : notarisations_being_built_[notarisation_collection_height_][hash])
      {
        notarisation_shares.insert({shares.first, shares.second.notarisation_share});
      }
      auto sig = notarisation_unit->ComputeAggregateSignature(notarisation_shares);
      assert(notarisation_unit->VerifyAggregateSignature(hash, sig));

      FETCH_LOG_INFO(LOGGING_NAME, "Notarisation built for block number ",
                     notarisation_collection_height_);
      assert(notarisations_built_[notarisation_collection_height_].find(hash) ==
             notarisations_built_[notarisation_collection_height_].end());
      notarisations_built_[notarisation_collection_height_][hash] = sig;
    }
  }

  return State::COMPLETE;
}

NotarisationService::State NotarisationService::OnComplete()
{
  FETCH_LOCK(mutex_);

  // Trim maps of unnecessary info
  auto const max_cache_size =
      ((active_notarisation_unit_->round_end() - active_notarisation_unit_->round_start()) + 1) * 3;
  TrimToSize(notarisations_being_built_, max_cache_size);
  TrimToSize(notarisations_built_, max_cache_size);

  // If chain has moved ahead faster while notarisations were being collected then reset
  // the block number collection is working on
  if (notarised_chain_height_ + 1 > notarisation_collection_height_)
  {
    notarisation_collection_height_ = notarised_chain_height_ + 1;
  }

  // If we have new notarisation keys and the next block to notarise is the end of the aeon then we
  // change keys - prevents switching out of keys too early if they are computed far in advance
  bool load_new_keys =
      new_keys && notarisation_collection_height_ == active_notarisation_unit_->round_end();
  if (load_new_keys || notarisation_collection_height_ > active_notarisation_unit_->round_end())
  {
    return State::KEY_ROTATION;
  }

  return State::COLLECT_NOTARISATIONS;
}

NotarisationService::BlockNotarisationShares NotarisationService::GetNotarisations(
    BlockNumber const &block_number)
{
  FETCH_LOCK(mutex_);
  if (notarisations_being_built_.find(block_number) == notarisations_being_built_.end())
  {
    return {};
  }
  return notarisations_being_built_.at(block_number);
}

void NotarisationService::NotariseBlock(Block const &block)
{
  FETCH_LOCK(mutex_);

  // If block number of previous block is greater than the heaviest notarised block observed so far
  // then reset to the block number of previous block
  if (block.block_number > notarised_chain_height_ + 1)
  {
    notarised_chain_height_ = block.block_number - 1;
  }

  // Return if block is too far in the past of the head of the notarised chain
  if (block.block_number < BlockNumberCutoff())
  {
    return;
  }
  // Return if block has already been notarised
  if (notarisations_built_.find(block.block_number) != notarisations_built_.end() &&
      notarisations_built_.at(block.block_number).find(block.hash) !=
          notarisations_built_.at(block.block_number).end())
  {
    return;
  }

  // If block is within current aeon then notarise now
  SharedAeonNotarisationUnit notarisation_unit;
  if (active_notarisation_unit_ && active_notarisation_unit_->CanSign() &&
      block.block_number >= active_notarisation_unit_->round_start() &&
      block.block_number <= active_notarisation_unit_->round_end())
  {
    notarisation_unit = active_notarisation_unit_;
  }

  // If block is not in current aeon then check previous notarisation unit
  if (previous_notarisation_unit_ && previous_notarisation_unit_->CanSign() &&
      block.block_number >= previous_notarisation_unit_->round_start() &&
      block.block_number <= previous_notarisation_unit_->round_end())
  {
    notarisation_unit = previous_notarisation_unit_;
  }

  if (!notarisation_unit)
  {
    return;
  }

  // Sign and verify own notarisation and then save for peers to query
  auto notarisation = notarisation_unit->Sign(block.hash);
  assert(notarisation_unit->Verify(block.hash, notarisation, endpoint_.GetAddress()));

  // Sign notarisation with ecdsa private key
  auto ecdsa_sig =
      certificate_->Sign((serializers::MsgPackSerializer() << block.hash << notarisation).data());
  notarisations_being_built_[block.block_number][block.hash].insert(
      {endpoint_.GetAddress(), SignedNotarisation(ecdsa_sig, notarisation)});

  FETCH_LOG_DEBUG(LOGGING_NAME, "Notarised block at height ", block.block_number);
}

void NotarisationService::SetAeonDetails(uint64_t round_start, uint64_t round_end,
                                         uint32_t                    threshold,
                                         AeonNotarisationKeys const &cabinet_public_keys)
{
  FETCH_LOCK(mutex_);

  // If no active notarisation has been set for this aeon then create new one just for verifying
  // notarisations in blocks. Not necessary but reduces verification time
  if (!active_notarisation_unit_)
  {
    active_notarisation_unit_ = std::make_shared<NotarisationManager>();
    std::map<MuddleAddress, NotarisationManager::PublicKey> public_notarisation_keys;
    for (auto const &key : cabinet_public_keys)
    {
      public_notarisation_keys.insert({key.first, key.second.first});
    }
    active_notarisation_unit_->SetAeonDetails(round_start, round_end, threshold,
                                              public_notarisation_keys);
    assert(!active_notarisation_unit_->CanSign());
  }
}

NotarisationService::AggregateSignature NotarisationService::GetAggregateNotarisation(
    Block const &block)
{
  FETCH_LOCK(mutex_);
  AggregateSignature notarisation;
  notarisation.first.clear();

  // Do not need to add notarisation if building on genesis block
  if (block.block_number != 0)
  {
    auto all_notarisations = notarisations_built_[block.block_number];
    if (all_notarisations.find(block.hash) != all_notarisations.end())
    {
      notarisation = all_notarisations.at(block.hash);
    }
  }
  return notarisation;
}

std::weak_ptr<core::Runnable> NotarisationService::GetWeakRunnable()
{
  return state_machine_;
}

uint64_t NotarisationService::BlockNumberCutoff() const
{
  if (notarised_chain_height_ < cutoff_)
  {
    return 0;
  }
  return notarised_chain_height_ - cutoff_;
}

NotarisationService::NotarisationResult NotarisationService::Verify(
    BlockNumber const &block_number, BlockHash const &block_hash,
    AggregateSignature const &notarisation)
{
  FETCH_LOCK(mutex_);

  // If block is within current aeon then notarise now
  SharedAeonNotarisationUnit notarisation_unit;
  if (active_notarisation_unit_ && block_number >= active_notarisation_unit_->round_start() &&
      block_number <= active_notarisation_unit_->round_end())
  {
    notarisation_unit = active_notarisation_unit_;
  }

  // If block is not in current aeon then check previous notarisation unit
  if (previous_notarisation_unit_ && block_number >= previous_notarisation_unit_->round_start() &&
      block_number <= previous_notarisation_unit_->round_end())
  {
    notarisation_unit = previous_notarisation_unit_;
  }

  if (!notarisation_unit)
  {
    return NotarisationResult::CAN_NOT_VERIFY;
  }

  // Check signature deserialises and that the signature was created with the correct number of
  // signers
  if (count(notarisation.second.begin(), notarisation.second.end(), 1) ==
      notarisation_unit->threshold())
  {
    if (notarisation_unit->VerifyAggregateSignature(block_hash, notarisation))
    {
      return NotarisationResult::PASS_VERIFICATION;
    }
  }
  return NotarisationResult::FAIL_VERIFICATION;
}

bool NotarisationService::Verify(BlockHash const &           block_hash,
                                 AggregateSignature const &  notarisation,
                                 AeonNotarisationKeys const &signed_notarisation_key,
                                 uint32_t                    threshold)
{
  std::vector<NotarisationManager::PublicKey> notarisation_keys;
  for (auto const &key_sig_pair : signed_notarisation_key)
  {
    notarisation_keys.push_back(key_sig_pair.second.first);
  }

  if (count(notarisation.second.begin(), notarisation.second.end(), 1) == threshold)
  {
    return NotarisationManager::VerifyAggregateSignature(block_hash, notarisation,
                                                         notarisation_keys);
  }
  return false;
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
    text = "Collecting notarisations shares";
    break;
  case NotarisationService::State::VERIFY_NOTARISATIONS:
    text = "Verifying notarisation shares";
    break;
  case NotarisationService::State::COMPLETE:
    text = "Completion state";
    break;
  }

  return text;
}
}  // namespace ledger
}  // namespace fetch
