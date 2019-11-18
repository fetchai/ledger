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

#include "ledger/consensus/simulated_pow_consensus.hpp"

#include <ctime>
#include <random>
#include <utility>

/**
 * This simulated pow consensus is used for testing, when it is not needed to
 * do the full scheme, as key generation takes a long time.
 *
 * In order to simulate pow, the consensus will probalistically generate blocks
 * so as to emulate pow. Blocks are all assumed to be valid
 *
 */

namespace {

constexpr char const *LOGGING_NAME = "SimulatedPOWConsensus";

using Consensus       = fetch::ledger::SimulatedPOWConsensus;

using fetch::ledger::MainChain;
using fetch::ledger::Block;

Consensus::Consensus(StakeManagerPtr stake, BeaconSetupServicePtr beacon_setup,
                     BeaconServicePtr beacon, MainChain const &chain, StorageInterface &storage,
                     Identity mining_identity, uint64_t aeon_period, uint64_t max_cabinet_size,
                     uint64_t block_interval_ms, NotarisationPtr notarisation)
  : storage_{storage}
  , stake_{std::move(stake)}
  , cabinet_creator_{std::move(beacon_setup)}
  , beacon_{std::move(beacon)}
  , chain_{chain}
  , mining_identity_{std::move(mining_identity)}
  , mining_address_{chain::Address(mining_identity_)}
  , aeon_period_{aeon_period}
  , max_cabinet_size_{max_cabinet_size}
  , block_interval_ms_{block_interval_ms}
  , notarisation_{std::move(notarisation)}
{
  assert(stake_);
}

void Consensus::UpdateCurrentBlock(Block const &current)
{
}

NextBlockPtr Consensus::GenerateNextBlock()
{
}

Consensus::CabinetPtr Consensus::GetCabinet(Block const &previous) const
{
}

uint32_t Consensus::GetThreshold(Block const &block) const
{
}

Block GetBlockPriorTo(Block const &current, MainChain const &chain)
{
}

Block GetBeginningOfAeon(Block const &current, MainChain const &chain)
{
}

bool Consensus::VerifyNotarisation(Block const &block) const
{
}

uint64_t Consensus::GetBlockGenerationWeight(Block const &previous, chain::Address const &address)
{
}

Consensus::WeightedQual QualWeightedByEntropy(Consensus::BlockEntropy::Cabinet const &cabinet,
                                              uint64_t                                entropy)
{
}

bool Consensus::ValidBlockTiming(Block const &previous, Block const &proposed) const
{
}

bool ShouldTriggerAeon(uint64_t block_number, uint64_t aeon_period)
{
}

bool Consensus::ShouldTriggerNewCabinet(Block const &block)
{
}

// Helper function to determine whether the block has been signed correctly
bool BlockSignedByQualMember(fetch::ledger::Block const &block)
{
}

bool ValidNotarisationKeys(Consensus::BlockEntropy::Cabinet const &             cabinet,
                           Consensus::BlockEntropy::AeonNotarisationKeys const &notarisation_keys)
{
}

bool Consensus::EnoughQualSigned(BlockEntropy const &block_entropy) const
{
}

Status Consensus::ValidBlock(Block const &current) const
{
}

void Consensus::Reset(StakeSnapshot const &snapshot, StorageInterface &storage)
{
}

void Consensus::Refresh()
{}

void Consensus::SetThreshold(double threshold)
{
}

void Consensus::SetCabinetSize(uint64_t size)
{
}

StakeManagerPtr Consensus::stake()
{
}

void Consensus::SetDefaultStartTime(uint64_t default_start_time)
{
  default_start_time_ = default_start_time;
}

void Consensus::AddCabinetToHistory(uint64_t block_number, CabinetPtr const &cabinet)
{
  cabinet_history_[block_number] = cabinet;
  TrimToSize(cabinet_history_, HISTORY_LENGTH);
}

