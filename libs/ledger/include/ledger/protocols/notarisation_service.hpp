#pragma once
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

#include "beacon/aeon.hpp"
#include "core/state_machine.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/protocols/notarisation_protocol.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/server.hpp"

namespace fetch {
namespace ledger {

class MainChain;

/**
 * Service which notarises a continuous chain of blocks. Proceeds onto next block height
 * as long as one block at the previous height, which references a previous notarised block,
 * is notarised
 */
class NotarisationService
{
public:
  constexpr static char const *LOGGING_NAME = "NotarisationService";

  enum class State
  {
    KEY_ROTATION,
    NOTARISATION_SYNCHRONISATION,
    COLLECT_NOTARISATIONS,
    VERIFY_NOTARISATIONS,
    COMPLETE
  };

  using BeaconManager                 = dkg::BeaconManager;
  using Block                         = ledger::Block;
  using BlockHash                     = Block::Hash;
  using StateMachine                  = core::StateMachine<State>;
  using Identity                      = Block::Identity;
  using BlockBody                     = Block::Body;
  using BlockHeight                   = uint64_t;
  using MuddleInterface               = muddle::MuddleInterface;
  using Endpoint                      = muddle::MuddleEndpoint;
  using Server                        = fetch::muddle::rpc::Server;
  using ServerPtr                     = std::shared_ptr<Server>;
  using AeonExecutionUnit             = beacon::AeonExecutionUnit;
  using SharedAeonExecutionUnit       = std::shared_ptr<AeonExecutionUnit>;
  using Signature                     = crypto::mcl::Signature;
  using NotarisationInformation       = std::unordered_map<BeaconManager::MuddleAddress, Signature>;
  using CallbackFunction              = std::function<void(BlockHash)>;
  using BlockNotarisationShares       = std::unordered_map<BlockHash, NotarisationInformation>;
  using BlockHeightNotarisationShares = std::map<BlockHeight, BlockNotarisationShares>;
  using BlockHeightGroupNotarisations =
      std::map<BlockHeight, std::unordered_map<BlockHash, Signature>>;

  NotarisationService()                            = delete;
  NotarisationService(NotarisationService const &) = delete;

  NotarisationService(MuddleInterface &muddle, MainChain &main_chain);

  /// State methods
  /// @{
  State OnKeyRotation();
  State OnNotarisationSynchronisation();
  State OnCollectNotarisations();
  State OnVerifyNotarisations();
  State OnComplete();
  /// @}

  /// Protocol endpoints
  /// @{
  BlockNotarisationShares GetNotarisations(BlockHeight const &height);
  /// @}

  /// Setup management
  /// @{
  void NewAeonExeUnit(SharedAeonExecutionUnit const &keys);
  void SetNotarisedBlockCallback(CallbackFunction callback);
  /// @}

  /// Call to notarise block
  /// @{
  void NotariseBlock(BlockBody const &block);
  /// @}

  /// Helper function
  /// @{
  uint64_t NextBlockHeight() const;
  uint64_t BlockNumberCutoff() const;
  /// @}

  std::vector<std::weak_ptr<core::Runnable>> GetWeakRunnables();

private:
  /// @{Networking
  Endpoint &                  endpoint_;
  ServerPtr                   rpc_server_{nullptr};
  muddle::rpc::Client         rpc_client_;
  NotarisationServiceProtocol notarisation_protocol_;
  service::Promise            notarisation_promise_;
  /// @}

  std::mutex                    mutex_;
  std::shared_ptr<StateMachine> state_machine_;

  MainChain &chain_;  ///< Ref to system chain

  /// Management of active DKG keys
  std::deque<SharedAeonExecutionUnit> aeon_exe_queue_;
  SharedAeonExecutionUnit             active_exe_unit_;

  /// @{Notarisations
  BlockHeightNotarisationShares
      notarisations_being_built_;  ///< Signature shares for blocks at a particular height
  BlockHeightGroupNotarisations
      notarisations_built_;  ///< Group signatures for blocks at a particular height
  BlockHeightGroupNotarisations
      detached_notarisations_built_;  ///< Group signatures for blocks without notarisated previous
  std::unordered_map<BlockHeight, uint32_t>
           previous_notarisation_rank_;  ///< Heighest rank notarised at a particular block height
  uint64_t highest_notarised_block_height_{0};  ///< Current highest notarised block height
  static const uint32_t cutoff_ = 2;            ///< Number of blocks behind
  /// @}

  CallbackFunction callback_;  ///< Callback for new block notarisation
};

}  // namespace ledger
}  // namespace fetch
