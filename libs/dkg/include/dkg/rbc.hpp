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
#include "rbc_envelop.hpp"

#include <bitset>

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

  RBC(Endpoint &endpoint, MuddleAddress address, CabinetMembers const &cabinet,
      std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)> call_back);

  // Operators
  void ResetCabinet();
  void SendRBroadcast(SerialisedMessage const &msg);

private:
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
    std::map<uint8_t, RBCMessage &> undelivered_msg;  ///< Undelivered messages indexed by tag
  };

  uint32_t           id_;  ///< Rank used in DKG (derived from position in current_cabinet_)
  uint8_t            msg_counter_;  ///< Counter for messages we have broadcasted
  std::vector<Party> parties_;      ///< Keeps track of messages from cabinet members
  std::unordered_map<uint64_t, Broadcast> broadcasts_;  ///< map from tag to broadcasts

  std::mutex mutex_flags_;      // protects access to Party message flags
  std::mutex mutex_deliver_;    // protects the delivered message queue
  std::mutex mutex_broadcast_;  // protects broadcasts_

  // For broadcast
  static constexpr uint16_t SERVICE_DKG       = 5001;
  static constexpr uint8_t  CHANNEL_BROADCAST = 2;  ///< Channel for reliable broadcast

  MuddleAddress const address_;   ///< Our muddle address
  Endpoint &          endpoint_;  ///< The muddle endpoint to communicate on
  CabinetMembers const
      &    current_cabinet_;  ///< The set of muddle addresses of the cabinet (including our own)
  uint32_t threshold_;  ///< Number of byzantine nodes (this is assumed to take the maximum allowed
                        ///< value satisying threshold_ < current_cabinet_.size()
  std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)>
                  deliver_msg_callback_;  ///< Callback for messages which have succeeded RBC protocol
  SubscriptionPtr rbc_subscription_;      ///< For receiving messages in the rbc channel

  void Send(RBCEnvelop const &env, MuddleAddress const &address);
  void Broadcast(RBCEnvelop const &env);
  void OnRBC(MuddleAddress const &from, RBCEnvelop const &envelop);
  void OnRBroadcast(std::shared_ptr<RBroadcast> msg_ptr, uint32_t sender_index);
  void OnREcho(std::shared_ptr<REcho> msg_ptr, uint32_t sender_index);
  void OnRReady(std::shared_ptr<RReady> msg_ptr, uint32_t sender_index);
  void OnRRequest(std::shared_ptr<RRequest> msg_ptr, uint32_t sender_index);
  void OnRAnswer(std::shared_ptr<RAnswer> msg_ptr, uint32_t sender_index);
  void Deliver(SerialisedMessage const &msg, uint32_t sender_index);

  static std::string MsgTypeToString(MsgType msg_type);
  uint32_t           CabinetIndex(MuddleAddress const &other_address) const;
  bool               CheckTag(RBCMessage &msg);
  bool               SetMbar(TagType tag, std::shared_ptr<RMessage> msg_ptr, uint32_t sender_index);
  bool               SetDbar(TagType tag, std::shared_ptr<RHash> msg_ptr);
  bool               ReceivedEcho(TagType tag, std::shared_ptr<REcho> msg_ptr);
  struct MsgCount    ReceivedReady(TagType tag, std::shared_ptr<RHash> msg_ptr);
  bool               SetPartyFlag(uint32_t sender_index, TagType tag, MsgType msg_type);
};
}  // namespace dkg
}  // namespace fetch
