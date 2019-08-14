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
#include "ledger/chain/address.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/muddle/rpc/server.hpp"
#include "rbc_envelope.hpp"

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

  RBC(Endpoint &endpoint, MuddleAddress address,
      std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)> call_back,
      uint8_t                                                                        channel = 2);

  // Operators
  void ResetCabinet(CabinetMembers const &cabinet);
  void SendRBroadcast(SerialisedMessage const &msg);

protected:
  enum class MsgType : uint8_t
  {
    R_SEND,
    R_ECHO,
    R_READY,
    R_REQUEST,
    R_ANSWER
  };

  struct MsgCount
  {
    uint64_t echo_count, ready_count;  ///< Count of RReady and RRecho messages
  };

  struct Broadcast
  {
    SerialisedMessage mbar;  ///< Original message broadcasted
    TruncatedHash     dbar;  ///< Hash of message
    std::unordered_map<TruncatedHash, MsgCount>
        msgs_count;  ///< Count of RBCMessages received for a given hash
  };

  struct Party
  {
    std::unordered_map<TagType, std::bitset<sizeof(MsgType) * 8>>
            flags;          ///< Marks for each message tag what messages have been received
    uint8_t deliver_s = 1;  ///< Counter for messages delivered - initialised to 1
    std::map<uint8_t, TagType>
        undelivered_msg;  ///< Undelivered message tags indexed by sequence counter
  };

  uint32_t           id_;  ///< Rank used in DKG (derived from position in current_cabinet_)
  uint8_t            msg_counter_;  ///< Counter for messages we have broadcasted
  std::vector<Party> parties_;      ///< Keeps track of messages from cabinet members
  std::unordered_map<TagType, Broadcast> broadcasts_;  ///< map from tag to broadcasts
  std::unordered_set<TagType>            delivered_;   ///< Tags of messages delivered

  std::mutex mutex_flags_;      ///< Protects access to Party message flags
  std::mutex mutex_deliver_;    ///< Protects the delivered message queue
  std::mutex mutex_broadcast_;  ///< Protects broadcasts_

  std::mutex mutex_current_cabinet_;

  // For broadcast
  static constexpr uint16_t SERVICE_DKG = 5001;
  uint8_t                   CHANNEL_BROADCAST;  ///< Channel for reliable broadcast

  MuddleAddress const address_;   ///< Our muddle address
  Endpoint &          endpoint_;  ///< The muddle endpoint to communicate on
  CabinetMembers
           current_cabinet_;  ///< The set of muddle addresses of the cabinet (including our own)
  uint32_t threshold_;  ///< Number of byzantine nodes (this is assumed to take the maximum allowed
                        ///< value satisying threshold_ < current_cabinet_.size()
  std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)>
                  deliver_msg_callback_;  ///< Callback for messages which have succeeded RBC protocol
  SubscriptionPtr rbc_subscription_;      ///< For receiving messages in the rbc channel

  void         Send(RBCEnvelope const &env, MuddleAddress const &address);
  virtual void Broadcast(RBCEnvelope const &env);
  virtual void OnRBC(MuddleAddress const &from, RBCEnvelope const &envelope);
  void         OnRBroadcast(RBroadcast const &msg, uint32_t sender_index);
  void         OnREcho(REcho const &msg, uint32_t sender_index);
  void         OnRReady(RReady const &msg, uint32_t sender_index);
  void         OnRRequest(RRequest const &msg, uint32_t sender_index);
  void         OnRAnswer(RAnswer const &msg, uint32_t sender_index);
  void         Deliver(SerialisedMessage const &msg, uint32_t sender_index);

  static std::string MsgTypeToString(MsgType msg_type);
  uint32_t           CabinetIndex(MuddleAddress const &other_address) const;
  bool BasicMsgCheck(MuddleAddress const &from, std::shared_ptr<RBCMessage> const &msg_ptr);
  bool CheckTag(RBCMessage const &msg);
  bool SetMbar(TagType tag, RMessage const &msg, uint32_t sender_index);
  bool SetDbar(TagType tag, RHash const &msg);
  bool ReceivedEcho(TagType tag, REcho const &msg);
  struct MsgCount ReceivedReady(TagType tag, RHash const &msg);
  bool            SetPartyFlag(uint32_t sender_index, TagType tag, MsgType msg_type);
};

TruncatedHash MessageHash(SerialisedMessage const &msg);
}  // namespace dkg
}  // namespace fetch
