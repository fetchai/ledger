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

#include "core/byte_array/const_byte_array.hpp"
#include "core/service_ids.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/address.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/rpc/server.hpp"
#include "rbc_messages.hpp"

#include <bitset>
#include <unordered_set>

namespace fetch {
namespace dkg {

/**
 * Reliable broadcast channel (RBC) is a protocol which ensures all honest
 * parties receive the same message in the presence of a threshold number of
 * Byzantine adversaries
 */
class RBC
{
public:
  using Endpoint        = muddle::MuddleEndpoint;
  using ConstByteArray  = byte_array::ConstByteArray;
  using MuddleAddress   = ConstByteArray;
  using CabinetMembers  = std::set<MuddleAddress>;
  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;
  using MessageType     = RBCMessage::MessageType;
  using HashFunction    = crypto::SHA256;
  using MessageHash     = byte_array::ByteArray;
  using CallbackFunction =
      std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)>;

  RBC(Endpoint &endpoint, MuddleAddress address, CallbackFunction call_back,
      uint16_t channel = CHANNEL_BROADCAST);

  // Operators
  bool ResetCabinet(CabinetMembers const &cabinet);
  void SendRBroadcast(SerialisedMessage const &msg);

protected:
  struct MessageCount
  {
    uint64_t echo_count{0};
    uint64_t ready_count{0};  ///< Count of RReady and RRecho messages
  };
  using MessageStatMap = std::unordered_map<MessageHash, MessageCount>;
  using FlagType       = std::bitset<sizeof(MessageType) * 8>;

  struct BroadcastMessage
  {
    SerialisedMessage original_message{};  ///< Original message broadcasted
    MessageHash       message_hash{};      ///< Hash of message
    MessageStatMap    msgs_count{};        ///< Count of RBCMessages received for a given hash
  };

  struct Party
  {
    std::unordered_map<TagType, FlagType>
            flags;          ///< Marks for each message tag what messages have been received
    uint8_t deliver_s = 1;  ///< Counter for messages delivered - initialised to 1
    std::map<uint8_t, TagType>
        undelivered_msg;  ///< Undelivered message tags indexed by sequence counter
  };
  using PartyList = std::vector<Party>;

  /// Sending messages
  /// @{
  void         Send(RBCMessage const &env, MuddleAddress const &address);
  virtual void Broadcast(RBCMessage const &env);
  /// @}

  /// Events
  /// @{
  virtual void OnRBC(MuddleAddress const &from, RBCMessage const &message);
  void         OnRBroadcast(RBCMessage const &msg, uint32_t sender_index);
  void         OnREcho(RBCMessage const &msg, uint32_t sender_index);
  void         OnRReady(RBCMessage const &msg, uint32_t sender_index);
  void         OnRRequest(RBCMessage const &msg, uint32_t sender_index);
  void         OnRAnswer(RBCMessage const &msg, uint32_t sender_index);
  void         Deliver(SerialisedMessage const &msg, uint32_t sender_index);
  /// @}

  /// Helper functions
  /// @{
  uint32_t            CabinetIndex(MuddleAddress const &other_address) const;
  bool                BasicMessageCheck(MuddleAddress const &from, RBCMessage const &msg);
  bool                CheckTag(RBCMessage const &msg);
  bool                SetMbar(TagType tag, RMessage const &msg, uint32_t sender_index);
  bool                SetDbar(TagType tag, RHash const &msg);
  bool                ReceivedEcho(TagType tag, RBCMessage const &msg);
  struct MessageCount ReceivedReady(TagType tag, RHash const &msg);
  bool                SetPartyFlag(uint32_t sender_index, TagType tag, MessageType msg_type);
  /// @}

  /// Variable Declarations
  /// @{
  uint16_t  channel_{CHANNEL_BROADCAST};
  uint32_t  id_;           ///< Rank used in DKG (derived from position in current_cabinet_)
  uint8_t   msg_counter_;  ///< Counter for messages we have broadcasted
  PartyList parties_;      ///< Keeps track of messages from cabinet members
  std::unordered_map<TagType, BroadcastMessage> broadcasts_;  ///< map from tag to broadcasts
  std::unordered_set<TagType>                   delivered_;   ///< Tags of messages delivered

  std::mutex mutex_flags_;      ///< Protects access to Party message flags
  std::mutex mutex_deliver_;    ///< Protects the delivered message queue
  std::mutex mutex_broadcast_;  ///< Protects broadcasts_

  // For broadcast
  MuddleAddress const address_;            ///< Our muddle address
  Endpoint &          endpoint_;           ///< The muddle endpoint to communicate on
  CabinetMembers      current_cabinet_;    ///< The set of muddle addresses of the
                                           ///< cabinet (including our own)
  uint32_t threshold_;                     ///< Number of byzantine nodes (this is assumed
                                           ///< to take the maximum allowed value satisying
                                           ///< threshold_ < current_cabinet_.size()
  CallbackFunction deliver_msg_callback_;  ///< Callback for messages which have succeeded
                                           ///< RBC protocol
  SubscriptionPtr rbc_subscription_;       ///< For receiving messages in the rbc channel

  /// @}
};

}  // namespace dkg
}  // namespace fetch
