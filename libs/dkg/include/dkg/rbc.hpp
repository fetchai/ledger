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
namespace rbc {

class RBC
{
public:
  using Endpoint        = muddle::MuddleEndpoint;
  using ConstByteArray  = byte_array::ConstByteArray;
  using MuddleAddress   = ConstByteArray;
  using CabinetMembers  = std::set<MuddleAddress>;
  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;

  explicit RBC(Endpoint &endpoint, MuddleAddress address, const CabinetMembers &cabinet,
               uint32_t threshold);
  void SendRBroadcast(const SerialisedMessage &msg);
  void OnRBC(MuddleAddress const &from, RBCEnvelop const &envelop,
             MuddleAddress const &transmitter);

private:
  // structs useful in RBC
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
    size_t echo_count, ready_count;
    MsgCount()
      : echo_count{0}
      , ready_count{0}
    {}
  };

  struct Broadcast
  {                                                     // information per tag
    SerialisedMessage                           mbar;  // message
    TruncatedHash                               dbar;  // hash of message
    std::unordered_map<TruncatedHash, MsgCount> msgs_count;
  };

  struct Party
  {  // information per party
    std::unordered_map<TagType, std::bitset<sizeof(MsgType) * 8>> flags;
    uint8_t                                                       deliver_s;
    std::map<uint8_t, RBCMessage &> undelivered_msg;  // indexed by tag
    Party()
    {
      deliver_s = 1;  // initialize sequence counter by 1
    }
  };

  uint32_t num_parties_{0};  ///< Number of participants in broadcast network
  uint32_t threshold_{0};    ///< Number of byzantine nodes

  uint32_t           id_;      ///< Should this match BLS ID (derived from muddle address?)
  uint8_t            seq_{0};  // sequence counter
  std::vector<Party> parties_;
  std::unordered_map<uint64_t, Broadcast> broadcasts_;  ///< map from tag to broadcasts
  std::mutex                              mutex_flags_;
  std::mutex                              mutex_deliver_;
  std::mutex                              mutex_broadcast_;

  // For broadcast
  static constexpr uint16_t SERVICE_DKG = 5001;
  static constexpr uint16_t CHANNEL_BROADCAST = 2;  ///< Replaces what was previously uint8_t channelId_

  // From before but useful
  MuddleAddress const address_;   ///< Our muddle address
  Endpoint &          endpoint_;  ///< The muddle endpoint to communicate on
  CabinetMembers
                  current_cabinet_;  ///< The set of muddle addresses of the cabinet (including our own)
  SubscriptionPtr rbc_subscription_;  ///< For receiving messages in the rbc channel

  void Send(const RBCEnvelop &env, const MuddleAddress &address);
  void Broadcast(const RBCEnvelop &env);
  void OnRBroadcast(std::shared_ptr<RBroadcast> msg_ptr, uint32_t sender_index);
  void OnREcho(std::shared_ptr<REcho> msg_ptr, uint32_t sender_index);
  void OnRReady(std::shared_ptr<RReady> msg_ptr, uint32_t sender_index);
  void OnRRequest(std::shared_ptr<RRequest> msg_ptr, uint32_t sender_index);
  void OnRAnswer(std::shared_ptr<RAnswer> msg_ptr, uint32_t sender_index);
  void Deliver(const SerialisedMessage &msg, uint32_t sender_index);

  static std::string MsgTypeToString(MsgType msg_type);
  uint32_t        CabinetIndex(const MuddleAddress &other_address) const;
  bool            CheckTag(RBCMessage &msg);
  bool            SetMbar(TagType tag, std::shared_ptr<RMessage> msg_ptr, uint32_t sender_index);
  bool            SetDbar(TagType tag, std::shared_ptr<RHash> msg_ptr);
  bool            ReceivedEcho(TagType tag, std::shared_ptr<REcho> msg_ptr);
  struct MsgCount ReceivedReady(TagType tag, std::shared_ptr<RHash> msg_ptr);
  bool            SetPartyFlag(uint32_t sender_index, TagType tag, MsgType msg_type);
};
}  // namespace rbc
}  // namespace dkg
}  // namespace fetch
