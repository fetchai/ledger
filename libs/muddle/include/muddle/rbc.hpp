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
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rbc_messages.hpp"

#include <atomic>
#include <bitset>
#include <unordered_set>

namespace fetch {
namespace muddle {

/**
 * Reliable broadcast channel (RBC) is a protocol which ensures all honest
 * parties receive the same message in the presence of a threshold number of
 * Byzantine adversaries
 */
class RBC
{
protected:
  struct MessageCount;
  struct BroadcastMessage;
  struct Party;

public:
  using Endpoint        = muddle::MuddleEndpoint;
  using ConstByteArray  = byte_array::ConstByteArray;
  using MuddleAddress   = ConstByteArray;
  using CabinetMembers  = std::set<MuddleAddress>;
  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;
  using MessageType     = RBCMessageType;
  using HashFunction    = crypto::SHA256;
  using HashDigest      = byte_array::ByteArray;
  using CallbackFunction =
      std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)>;
  using MessageStatMap = std::unordered_map<HashDigest, MessageCount>;
  using FlagType       = std::bitset<sizeof(MessageType) * 8>;
  using PartyList      = std::vector<Party>;
  using IdType         = uint32_t;
  using CounterType    = uint8_t;

  RBC(Endpoint &endpoint, MuddleAddress address, CallbackFunction call_back,
      uint16_t channel = CHANNEL_RBC_BROADCAST, bool ordered_delivery = true);

  /// RBC Operation
  /// @{
  bool ResetCabinet(CabinetMembers const &cabinet);
  void Broadcast(SerialisedMessage const &msg);
  /// @}

protected:
  /// Structs used for the message tracking
  /// @{
  struct MessageCount
  {
    uint64_t echo_count{0};
    uint64_t ready_count{0};  ///< Count of RReady and RRecho messages
  };

  struct BroadcastMessage
  {
    SerialisedMessage original_message{};  ///< Original message broadcasted
    HashDigest        message_hash{};      ///< Hash of message
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
  /// @}

  /// Events
  /// @{
  // Unsafe
  virtual void OnRBC(MuddleAddress const &from, RBCMessage const &message);
  void         OnRBroadcast(MessageBroadcast const &msg, uint32_t sender_index);
  void         OnREcho(MessageEcho const &msg, uint32_t sender_index);
  void         OnRReady(MessageReady const &msg, uint32_t sender_index);
  void         OnRRequest(MessageRequest const &msg, uint32_t sender_index);
  void         OnRAnswer(MessageAnswer const &msg, uint32_t sender_index);
  /// @}

  /// Message communication - not thread safe.
  /// @{
  void         Send(RBCMessage const &env, MuddleAddress const &address);
  virtual void InternalBroadcast(RBCMessage const &env);
  void         Deliver(SerialisedMessage const &msg, uint32_t sender_index);

  Endpoint &endpoint()
  {
    return endpoint_;
  }
  /// @}

  uint32_t id() const
  {
    return id_;
  }

  uint8_t message_counter() const
  {
    return msg_counter_;
  }

  void increase_message_counter()
  {
    ++msg_counter_;
  }

  CabinetMembers current_cabinet()
  {
    return current_cabinet_;
  }

  /// Helper functions - not thread safe.
  /// @{
  uint32_t            CabinetIndex(MuddleAddress const &other_address) const;
  bool                BasicMessageCheck(MuddleAddress const &from, RBCMessage const &msg);
  bool                CheckTag(RBCMessage const &msg);
  bool                SetMbar(TagType tag, MessageContents const &msg, uint32_t sender_index);
  bool                SetDbar(TagType tag, MessageHash const &msg);
  bool                ReceivedEcho(TagType tag, MessageEcho const &msg);
  struct MessageCount ReceivedReady(TagType tag, MessageHash const &msg);
  bool                SetPartyFlag(uint32_t sender_index, TagType tag, MessageType msg_type);
  /// @}

  /// Mutex setup that allows easy debugging of deadlocks
  /// @{
  mutable mutex::Mutex lock_{__LINE__, __FILE__};
  /// @}
private:
  /// Variable Declarations
  /// @{
  uint16_t channel_{CHANNEL_RBC_BROADCAST};

  std::atomic<uint32_t> id_{0};  ///< Rank used in RBC (derived from position in current_cabinet_)
  std::atomic<uint8_t>  msg_counter_{0};  ///< Counter for messages we have broadcasted
  PartyList             parties_;         ///< Keeps track of messages from cabinet members
  std::unordered_map<TagType, BroadcastMessage> broadcasts_;  ///< map from tag to broadcasts
  bool                                          ordered_delivery_;

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

}  // namespace muddle
}  // namespace fetch
