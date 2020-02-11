#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/sha256.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

#include "network/service/protocol.hpp"

#include "moment/clocks.hpp"
#include "moment/deadline_timer.hpp"

// TODO(HUT): not needed with reworking
#include "muddle/rbc.hpp"

#include "muddle/question_struct.hpp"

#include <map>
#include <random>
#include <set>

namespace fetch {
namespace muddle {

/**
 * Punishment broadcast channel aims to answer a 'question' posed to peers. For a unique question
 * hash, it will synchronise with predefined peers (the cabinet) by leveraging cryptographic
 * signatures to trust that information about other peers is true.
 *
 * To do so, it constructs a table of 'answers' which it first populates with its own answers, then
 * pulls from peers to populate with their answers. For peers to respond with two different answers
 * to the same question (which is signed) is invalid and should result in punishment.
 *
 * Once the answers in the table have enough 'seen' signatures (threshold), it can be dispatched to
 * the callback.
 */
class PunishmentBroadcastChannel : public service::Protocol, public BroadcastChannelInterface
{
public:
  enum class State
  {
    INIT,
    RESOLVE_PROMISES,
  };

  using CertificatePtr       = std::shared_ptr<fetch::crypto::Prover>;
  using Endpoint             = muddle::MuddleEndpoint;
  using ConstByteArray       = byte_array::ConstByteArray;
  using MuddleAddress        = ConstByteArray;
  using CabinetMembers       = std::set<MuddleAddress>;
  using CabinetMembersVector = std::vector<MuddleAddress>;
  using Subscription         = muddle::Subscription;
  using SubscriptionPtr      = std::shared_ptr<muddle::Subscription>;
  using HashFunction         = crypto::SHA256;
  using HashDigest           = byte_array::ByteArray;
  using CallbackFunction =
      std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)>;
  using Server          = fetch::muddle::rpc::Server;
  using ServerPtr       = std::shared_ptr<Server>;
  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;

  // The only RPC function exposed to peers
  enum
  {
    PULL_INFO_FROM_PEER = 1
  };

  QuestionStruct AllowPeerPull();

  PunishmentBroadcastChannel(Endpoint &endpoint, MuddleAddress address, CallbackFunction call_back,
                             CertificatePtr certificate, uint16_t channel = CHANNEL_RBC_BROADCAST,
                             bool ordered_delivery = true);

  ~PunishmentBroadcastChannel() override;

  // Interface methods
  bool ResetCabinet(CabinetMembers const &cabinet) override;

  /**
   * Set/reset the question, saving the old question, so that peer who have not yet
   * reset can still access this information
   *
   */
  void SetQuestion(ConstByteArray const &question, ConstByteArray const &answer) override;

  void SetQuestion(std::string const &question, std::string const &answer) /* override */;

  void Enable(bool enable) override;

  /**
   * The channel needs to be driven to resolve network events
   */
  std::weak_ptr<core::Runnable> GetRunnable() override;

private:
  static constexpr char const *LOGGING_NAME = "PunishmentChannel";

  /**
   * Determine whether the current question has been answered by all peers.
   * If not, continue to try to resolve answers
   */
  bool AnsweredQuestion();

  PunishmentBroadcastChannel::State OnInit();

  /**
   * Attempt to resolve the network promises and add them to our table,
   * eventually timing out.
   */
  PunishmentBroadcastChannel::State OnResolvePromises();

  Endpoint &          endpoint_;              ///< The muddle endpoint to communicate on
  MuddleAddress const address_;               ///< Our muddle address
  CallbackFunction    deliver_msg_callback_;  ///< Callback for messages which have succeeded
  const uint16_t      channel_;
  CertificatePtr      certificate_;

  ServerPtr           rpc_server_{nullptr};
  muddle::rpc::Client rpc_client_;

  bool enabled_ = true;

  // For broadcast
  CabinetMembers current_cabinet_;  ///< The set of muddle addresses of the
                                    ///< cabinet (including our own)
  CabinetMembersVector
      current_cabinet_vector_;  ///< The current cabinet shuffled into a vector.
                                ///< Used to randomly select cabinet members without replacement
  uint32_t threshold_;          ///< Number of byzantine nodes (this is assumed
                                ///< to take the maximum allowed value satisfying
                                ///< threshold_ < current_cabinet_.size()

  moment::ClockPtr      clock_ = moment::GetClock("muddle:pbc", moment::ClockType::SYSTEM);
  moment::DeadlineTimer time_to_wait_{"muddle:pbc"};

  const uint8_t                                           concurrent_promises_allowed = 2;
  const uint16_t                                          REASONABLE_NETWORK_DELAY_MS = 500;
  std::vector<std::pair<MuddleAddress, service::Promise>> network_promises_;

  /// @}
  StateMachinePtr state_machine_;

  // ONLY the question (and previous question if this is enabled) needs to be locked
  mutable Mutex  lock_;
  QuestionStruct question_;
  QuestionStruct previous_question_;
};
}  // namespace muddle

namespace serializers {

template <typename D>
struct MapSerializer<muddle::QuestionStruct, D>
{
public:
  using Type       = muddle::QuestionStruct;
  using DriverType = D;

  static uint8_t const TABLE    = 1;
  static uint8_t const QUESTION = 2;
  static uint8_t const CABINET  = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &table)
  {
    auto map = map_constructor(3);
    map.Append(TABLE, table.table_);
    map.Append(QUESTION, table.question_);
    map.Append(CABINET, table.cabinet_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &table)
  {
    map.ExpectKeyGetValue(TABLE, table.table_);
    map.ExpectKeyGetValue(QUESTION, table.question_);
    map.ExpectKeyGetValue(CABINET, table.cabinet_);
  }
};

template <typename D>
struct MapSerializer<muddle::QuestionStruct::AnswerAndSeen, D>
{
public:
  using Type       = muddle::QuestionStruct::AnswerAndSeen;
  using DriverType = D;

  static uint8_t const ANSWER    = 1;
  static uint8_t const SIGNATURE = 2;
  static uint8_t const SEEN      = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &tuple)
  {
    auto map = map_constructor(3);
    map.Append(ANSWER, std::get<0>(tuple));
    map.Append(SIGNATURE, std::get<1>(tuple));
    map.Append(SEEN, std::get<2>(tuple));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &tuple)
  {
    map.ExpectKeyGetValue(ANSWER, std::get<0>(tuple));
    map.ExpectKeyGetValue(SIGNATURE, std::get<1>(tuple));
    map.ExpectKeyGetValue(SEEN, std::get<2>(tuple));
  }
};

}  // namespace serializers

}  // namespace fetch
