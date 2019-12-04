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

#include "beacon/beacon_setup_service.hpp"
#include "beacon/block_entropy.hpp"
#include "beacon/notarisation_manager.hpp"
#include "core/state_machine.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/protocols/notarisation_protocol.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/server.hpp"

namespace fetch {
namespace ledger {

class MainChain;

/**
 * Service which notarises valid blocks, and verifies notarisations inside blocks. View of chain is
 * based on what the service has been called to notarise and collects notarisation shares from peers
 * to compute aggregate notarisation of blocks with the highest block number in chain
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

  enum class NotarisationResult
  {
    CAN_NOT_VERIFY,
    PASS_VERIFICATION,
    FAIL_VERIFICATION
  };

  struct SignedNotarisation
  {
    byte_array::ConstByteArray ecdsa_signature;
    crypto::mcl::Signature     notarisation_share;

    SignedNotarisation() = default;
    SignedNotarisation(byte_array::ConstByteArray ecdsa_sig, crypto::mcl::Signature notarisation)
      : ecdsa_signature{std::move(ecdsa_sig)}
      , notarisation_share{std::move(notarisation)}
    {}
  };

  using ConstByteArray                = byte_array::ConstByteArray;
  using MuddleAddress                 = ConstByteArray;
  using StateMachine                  = core::StateMachine<State>;
  using Block                         = ledger::Block;
  using BlockHash                     = Block::Hash;
  using Identity                      = Block::Identity;
  using BlockNumber                   = uint64_t;
  using BlockWeight                   = Block::Weight;
  using BlockEntropy                  = beacon::BlockEntropy;
  using AeonNotarisationKeys          = BlockEntropy::AeonNotarisationKeys;
  using BeaconSetupService            = beacon::BeaconSetupService;
  using MuddleInterface               = muddle::MuddleInterface;
  using Endpoint                      = muddle::MuddleEndpoint;
  using Server                        = muddle::rpc::Server;
  using ServerPtr                     = std::shared_ptr<Server>;
  using AeonNotarisationUnit          = ledger::NotarisationManager;
  using SharedAeonNotarisationUnit    = std::shared_ptr<AeonNotarisationUnit>;
  using Signature                     = NotarisationManager::Signature;
  using AggregateSignature            = NotarisationManager::AggregateSignature;
  using Certificate                   = crypto::Prover;
  using CertificatePtr                = std::shared_ptr<Certificate>;
  using CallbackFunction              = std::function<void(BlockHash)>;
  using NotarisationShares            = std::unordered_map<MuddleAddress, SignedNotarisation>;
  using BlockNotarisationShares       = std::unordered_map<BlockHash, NotarisationShares>;
  using BlockAggregateNotarisations   = std::unordered_map<BlockHash, AggregateSignature>;
  using BlockHeightNotarisationShares = std::map<BlockNumber, BlockNotarisationShares>;
  using BlockHeightGroupNotarisations = std::map<BlockNumber, BlockAggregateNotarisations>;

  NotarisationService()                            = delete;
  NotarisationService(NotarisationService const &) = delete;

  NotarisationService(MuddleInterface &muddle, CertificatePtr certificate,
                      BeaconSetupService &beacon_setup);

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
  BlockNotarisationShares GetNotarisations(BlockNumber const &block_number);
  /// @}

  /// Calls from other services
  /// @{
  void               NotariseBlock(Block const &block);
  void               SetAeonDetails(uint64_t round_start, uint64_t round_end, uint32_t threshold,
                                    AeonNotarisationKeys const &cabinet_public_keys);
  AggregateSignature GetAggregateNotarisation(Block const &block);
  /// @}

  /// Verifying notarised blocks
  /// @{
  NotarisationResult Verify(BlockNumber const &block_number, BlockHash const &block_hash,
                            AggregateSignature const &notarisation);
  static bool        Verify(BlockHash const &block_hash, AggregateSignature const &notarisation,
                            AeonNotarisationKeys const &signed_notarisation_key, uint32_t threshold);
  /// @}

  std::weak_ptr<core::Runnable> GetWeakRunnable();

private:
  /// Helper function
  /// @{
  uint64_t BlockNumberCutoff() const;
  /// @}

  /// @{Networking
  Endpoint &                  endpoint_;
  ServerPtr                   rpc_server_{nullptr};
  muddle::rpc::Client         rpc_client_;
  NotarisationServiceProtocol notarisation_protocol_;
  service::Promise            notarisation_promise_;
  /// @}

  std::mutex                    mutex_;
  CertificatePtr                certificate_;
  std::shared_ptr<StateMachine> state_machine_;

  /// @{Management of active notarisation keys
  bool                                   new_keys{false};
  std::deque<SharedAeonNotarisationUnit> aeon_notarisation_queue_;
  SharedAeonNotarisationUnit             active_notarisation_unit_;
  SharedAeonNotarisationUnit             previous_notarisation_unit_;
  /// @}

  /// @{Notarisations
  BlockHeightNotarisationShares
      notarisations_being_built_;  ///< Signature shares for blocks at a particular block number
  BlockHeightGroupNotarisations
           notarisations_built_;  ///< Group signatures for blocks at a particular block number
  uint64_t notarised_chain_height_{0};          ///< Current highest notarised block number in chain
  uint64_t notarisation_collection_height_{0};  ///< Block number current collecting signatures for
  static const uint32_t cutoff_ = 2;            ///< Number of blocks behind
  /// @}
};
}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::NotarisationService::SignedNotarisation, D>
{
public:
  using Type       = ledger::NotarisationService::SignedNotarisation;
  using DriverType = D;

  static uint8_t const SIGNATURE    = 0;
  static uint8_t const NOTARISATION = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &member)
  {
    auto map = map_constructor(2);

    map.Append(SIGNATURE, member.ecdsa_signature);
    map.Append(NOTARISATION, member.notarisation_share);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &member)
  {
    map.ExpectKeyGetValue(SIGNATURE, member.ecdsa_signature);
    map.ExpectKeyGetValue(NOTARISATION, member.notarisation_share);
  }
};
}  // namespace serializers
}  // namespace fetch
