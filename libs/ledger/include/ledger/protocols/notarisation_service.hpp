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

#include "core/state_machine.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/consensus/consensus.hpp"
#include "ledger/protocols/notarisation_protocol.hpp"

#include "beacon/aeon.hpp"

namespace fetch {
namespace ledger {

/**
 * Assume for now blocks arrive in order. Should not notarise a new block which references an
 * non-fully notarised block
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

  using BeaconManager = dkg::BeaconManager;
  // using BeaconServicePtr = std::shared_ptr<beacon::BeaconService>;
  using Block                   = ledger::Block;
  using BlockHash               = Block::Hash;
  using StateMachine            = core::StateMachine<State>;
  using Identity                = Block::Identity;
  using BlockBody               = Block::Body;
  using BlockHeight             = uint64_t;
  using MuddleInterface         = muddle::MuddleInterface;
  using Endpoint                = muddle::MuddleEndpoint;
  using Server                  = fetch::muddle::rpc::Server;
  using ServerPtr               = std::shared_ptr<Server>;
  using AeonExecutionUnit       = beacon::AeonExecutionUnit;
  using SharedAeonExecutionUnit = std::shared_ptr<AeonExecutionUnit>;
  using Signature               = crypto::mcl::Signature;
  using NotarisationInformation = std::unordered_map<BeaconManager::MuddleAddress, Signature>;

  NotarisationService()                            = delete;
  NotarisationService(NotarisationService const &) = delete;

  NotarisationService(MuddleInterface &muddle);

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
  std::unordered_map<BlockHash, NotarisationInformation> GetNotarisations(
      BlockHeight const &height);
  /// @}

  void                                       NotariseBlock(BlockBody const &block);
  std::vector<std::weak_ptr<core::Runnable>> GetWeakRunnables();

private:
  Endpoint &endpoint_;

  ServerPtr           rpc_server_{nullptr};
  muddle::rpc::Client rpc_client_;

  NotarisationServiceProtocol notarisation_protocol_;

  std::mutex                          mutex_;
  std::deque<SharedAeonExecutionUnit> aeon_exe_queue_;
  SharedAeonExecutionUnit             active_exe_unit_;

  std::map<BlockHeight, std::unordered_map<BlockHash, NotarisationInformation>>
                                                                  notarisations_being_built_;
  std::map<BlockHeight, std::unordered_map<BlockHash, Signature>> notarisations_built_;
  service::Promise                                                notarisation_promise_;

  std::unordered_map<BlockHeight, uint32_t> previous_notarisation_rank_;
  uint64_t                                  highest_notarised_block_height_{0};
  static const uint32_t                     cutoff = 2;  // Number of blocks behind

  std::shared_ptr<StateMachine> state_machine_;
};
}  // namespace ledger

namespace serializers {
// TODO(JMW): Serialisers for the notarisations
}
}  // namespace fetch
