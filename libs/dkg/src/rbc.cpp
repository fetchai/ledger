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

#include "core/logging.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "dkg/rbc.hpp"

namespace fetch {
namespace dkg {
namespace rbc {

constexpr char const *LOGGING_NAME = "RBC";

// Function to compute truncation of message hash to 8 bytes. Truncation from left side.
TruncatedHash MessageHash(const SerialisedMessage &msg)
{
  byte_array::ByteArray msg_hash_256{crypto::Hash<crypto::SHA256>(msg)};
  return msg_hash_256.SubArray(24);
}

std::string RBC::MsgTypeToString(MsgType msg_type)
{
  switch (msg_type)
  {
  case MsgType::R_SEND:
    return "r_send";
  case MsgType::R_ECHO:
    return "r_echo";
  case MsgType::R_READY:
    return "r_ready";
  case MsgType::R_REQUEST:
    return "r_request";
  case MsgType::R_ANSWER:
    return "r_answer";
  default:
    assert(false);
  }
}

RBC::RBC(Endpoint &endpoint, MuddleAddress address, const CabinetMembers &cabinet,
         uint32_t threshold)
  : address_{std::move(address)}
  , endpoint_{endpoint}
  , current_cabinet_{cabinet}
  , rbc_subscription_(endpoint.Subscribe(SERVICE_DKG, CHANNEL_BROADCAST))
  , threshold_{threshold}
{
  num_parties_ = static_cast<uint32_t>(current_cabinet_.size());
  assert(num_parties_ > 0);
  auto it{current_cabinet_.find(address_)};
  assert(it != current_cabinet_.end());
  id_ = static_cast<uint32_t>(
      std::distance(current_cabinet_.begin(), current_cabinet_.find(address_)));
  assert(num_parties_ > 3 * threshold_);
  parties_.resize(num_parties_);

  // set subscription for rbc
  rbc_subscription_->SetMessageHandler([this](MuddleAddress const &from, uint16_t, uint16_t,
                                              uint16_t, muddle::Packet::Payload const &payload,
                                              MuddleAddress transmitter) {
    RBCSerializer serialiser(payload);

    // deserialize the RBCEnvelop
    RBCEnvelop env;
    serialiser >> env;

    // dispatch the event
    OnRBC(from, env, transmitter);
  });
}

void RBC::Send(const RBCEnvelop &env, const MuddleAddress &address)
{
  // Serialise the RBCEnvelop
  RBCSerializerCounter env_counter;
  env_counter << env;

  RBCSerializer env_serializer;
  env_serializer.Reserve(env_counter.size());
  env_serializer << env;

  endpoint_.Send(address, SERVICE_DKG, CHANNEL_BROADCAST, env_serializer.data());
}

void RBC::Broadcast(const RBCEnvelop &env)
{
  // Serialise the RBCEnvelop
  RBCSerializerCounter env_counter;
  env_counter << env;

  RBCSerializer env_serializer;
  env_serializer.Reserve(env_counter.size());
  env_serializer << env;

  endpoint_.Broadcast(SERVICE_DKG, CHANNEL_BROADCAST, env_serializer.data());
}

void RBC::SendRBroadcast(const SerialisedMessage &msg)
{
  RBCEnvelop env{RBroadcast(CHANNEL_BROADCAST, id_, ++seq_, msg)};
  Broadcast(env);
  OnRBroadcast(std::dynamic_pointer_cast<RBroadcast>(env.getMessage()), id_);
}

bool RBC::SetMbar(TagType tag, std::shared_ptr<RMessage> msg_ptr, uint32_t sender_index)
{
  std::lock_guard<std::mutex> lock(mutex_broadcast_);
  if (broadcasts_[tag].mbar.empty())
  {
    broadcasts_[tag].mbar = msg_ptr->Message();
    return true;
  }
  if (broadcasts_[tag].mbar != msg_ptr->Message())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", id_, " received bad r-send message from ", sender_index);
  }
  return false;
}

bool RBC::SetDbar(TagType tag, std::shared_ptr<RHash> msg_ptr)
{
  std::lock_guard<std::mutex> lock(mutex_broadcast_);
  broadcasts_[tag].dbar = msg_ptr->Hash();
  TruncatedHash msg_hash;
  if (!broadcasts_[tag].mbar.empty())
  {
    msg_hash = MessageHash(broadcasts_[tag].mbar);
  }
  return msg_hash == msg_ptr->Hash();
}

bool RBC::ReceivedEcho(TagType tag, std::shared_ptr<REcho> msg_ptr)
{
  std::lock_guard<std::mutex> lock(mutex_broadcast_);
  auto &                      msg_count = broadcasts_[tag].msgs_count[msg_ptr->Hash()];
  msg_count.echo_count++;
  return (msg_count.echo_count == current_cabinet_.size() - threshold_ and msg_count.ready_count <= threshold_);
}

struct RBC::MsgCount RBC::ReceivedReady(TagType tag, std::shared_ptr<RHash> msg_ptr)
{
  std::lock_guard<std::mutex> lock(mutex_broadcast_);
  auto &                      msg_count = broadcasts_[tag].msgs_count[msg_ptr->Hash()];
  msg_count.ready_count++;
  MsgCount res = msg_count;
  return res;
}

void RBC::OnRBC(MuddleAddress const &from, RBCEnvelop const &envelop,
                MuddleAddress const &transmitter)
{
  auto     msg_ptr = envelop.getMessage();
  uint32_t sender_index{CabinetIndex(from)};
  switch (msg_ptr->Type())
  {
  case RBCMessage::MessageType::RBROADCAST:
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RBroadcast from node ", sender_index);
    OnRBroadcast(std::dynamic_pointer_cast<RBroadcast>(msg_ptr), sender_index);
    break;
  case RBCMessage::MessageType::RECHO:
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received REcho from node ", sender_index);
    OnREcho(std::dynamic_pointer_cast<REcho>(msg_ptr), sender_index);
    break;
  case RBCMessage::MessageType::RREADY:
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RReady from node ", sender_index);
    OnRReady(std::dynamic_pointer_cast<RReady>(msg_ptr), sender_index);
    break;
  case RBCMessage::MessageType::RREQUEST:
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RRequest from node ", sender_index);
    OnRRequest(std::dynamic_pointer_cast<RRequest>(msg_ptr), sender_index);
    break;
  case RBCMessage::MessageType::RANSWER:
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RAnswer from node ", sender_index);
    OnRAnswer(std::dynamic_pointer_cast<RAnswer>(msg_ptr), sender_index);
    break;
  default:
    FETCH_LOG_ERROR(LOGGING_NAME, "Node: ", id_, " can not process payload from node ",
                    sender_index);
  }
}

void RBC::OnRBroadcast(const std::shared_ptr<RBroadcast> msg_ptr, uint32_t sender_index)
{
  uint64_t tag{msg_ptr->Tag()};
  if (!SetPartyFlag(sender_index, tag, MsgType::R_SEND))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with sequence ", msg_ptr->SeqCounter(), " and id ",
                   msg_ptr->Id());
    return;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "onRBroadcast: Node ", id_, " received msg ", tag, " from node ",
                 sender_index, " with sequence ", msg_ptr->SeqCounter(), " and id ", msg_ptr->Id());
  if (sender_index == msg_ptr->Id())
  {
    if (SetMbar(tag, msg_ptr, sender_index))
    {
      broadcasts_[tag].mbar = msg_ptr->Message();
      RBCEnvelop env{REcho{msg_ptr->ChannelId(), msg_ptr->Id(), msg_ptr->SeqCounter(),
                           MessageHash(msg_ptr->Message())}};
      Broadcast(env);
      OnREcho(std::dynamic_pointer_cast<REcho>(env.getMessage()), id_);  // self sending.
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_,
                   " received wrong r-send msg from node ", sender_index, " for msg ", tag,
                   " with id ", msg_ptr->Id());
  }
}

void RBC::OnREcho(const std::shared_ptr<REcho> msg_ptr, uint32_t sender_index)
{
  uint64_t tag{msg_ptr->Tag()};
  if (!SetPartyFlag(sender_index, tag, MsgType::R_ECHO))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onREcho: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with sequence ", msg_ptr->SeqCounter(), " and id ",
                   msg_ptr->Id());
    return;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "onREcho: Node ", id_, " received msg ", tag, " from node ",
                 sender_index, " with sequence ", msg_ptr->SeqCounter(), " and id ", msg_ptr->Id());
  if (ReceivedEcho(tag, msg_ptr))
  {
    RBCEnvelop env{
        RReady{msg_ptr->ChannelId(), msg_ptr->Id(), msg_ptr->SeqCounter(), msg_ptr->Hash()}};
    Broadcast(env);
    OnRReady(std::dynamic_pointer_cast<RReady>(env.getMessage()), id_);  // self sending.
  }
}

void RBC::OnRReady(const std::shared_ptr<RReady> msg_ptr, uint32_t sender_index)
{
  uint64_t tag{msg_ptr->Tag()};
  if (!SetPartyFlag(sender_index, tag, MsgType::R_READY))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRReady: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with sequence ", msg_ptr->SeqCounter(), " and id ",
                   msg_ptr->Id());
    return;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "onRReady: Node ", id_, " received msg ", tag, " from node ",
                 sender_index, " with sequence ", msg_ptr->SeqCounter(), " and id ", msg_ptr->Id());
  auto msgsCount = ReceivedReady(tag, msg_ptr);
  if (threshold_ > 0 and msgsCount.ready_count == threshold_ + 1 and
      msgsCount.echo_count < (num_parties_ - threshold_))
  {
    RBCEnvelop env{
        RReady{msg_ptr->ChannelId(), msg_ptr->Id(), msg_ptr->SeqCounter(), msg_ptr->Hash()}};
    Broadcast(env);
    OnRReady(std::dynamic_pointer_cast<RReady>(env.getMessage()), id_);  // self sending.
  }
  else if (threshold_ > 0 and msgsCount.ready_count == 2 * threshold_ + 1)
  {
    broadcasts_[tag].dbar = msg_ptr->Hash();

    TruncatedHash msg_hash;
    if (!SetDbar(tag, msg_ptr))
    {
      RBCEnvelop env{RRequest{msg_ptr->ChannelId(), msg_ptr->Id(), msg_ptr->SeqCounter()}};

      uint32_t counter{0};
      auto     im{current_cabinet_.begin()};
      assert(2 * threshold_ + 1 <= current_cabinet_.size());
      while (counter < 2 * threshold_ + 1)
      {
        if (*im != address_)
        {
          Send(env, *im);
          ++counter;
          ++im;
        }
      }
    }
    else if (CheckTag(*msg_ptr) && msg_ptr->Id() != id_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", tag, " with sequence ",
                     msg_ptr->SeqCounter(), " and id ", msg_ptr->Id());
      Deliver(broadcasts_[tag].mbar, msg_ptr->Id());
    }
  }
}

void RBC::OnRRequest(const std::shared_ptr<RRequest> msg_ptr, uint32_t sender_index)
{
  uint64_t tag{msg_ptr->Tag()};
  if (!SetPartyFlag(sender_index, tag, MsgType::R_REQUEST))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRRequest: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with sequence ", msg_ptr->SeqCounter(), " and id ",
                   msg_ptr->Id());
    return;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "onRRequest: Node ", id_, " received msg ", tag, " from node ",
                 sender_index, " with sequence ", msg_ptr->SeqCounter(), " and id ", msg_ptr->Id());
  if (!broadcasts_[tag].mbar.empty())
  {
    RBCEnvelop env{RAnswer{msg_ptr->ChannelId(), msg_ptr->Id(), msg_ptr->SeqCounter(),
                           broadcasts_[tag].mbar}};

    auto im = std::next(current_cabinet_.begin(), sender_index);
    Send(env, *im);
  }
}

void RBC::OnRAnswer(const std::shared_ptr<RAnswer> msg_ptr, uint32_t sender_index)
{
  uint64_t tag{msg_ptr->Tag()};
  if (!SetPartyFlag(sender_index, tag, MsgType::R_ANSWER))
  {
    return;
  }
  // If have not set dbar then we did not send a request message
  if (broadcasts_[tag].dbar == 0)
  {
    return;
  }
  // Check the hash of the message
  TruncatedHash msg_hash{MessageHash(msg_ptr->Message())};
  if (msg_hash == broadcasts_[tag].dbar)
  {
    if (broadcasts_[tag].mbar.empty())
    {
      broadcasts_[tag].mbar = msg_ptr->Message();
    }
    else
    {
      // TODO: Double check this part of protocol
      broadcasts_[tag].mbar = msg_ptr->Message();
    }
  }
  else
  {
    // Hash does not match and we received a bad answer
    FETCH_LOG_WARN(LOGGING_NAME, "onRAnswer: Node ", id_, " received bad r-answer from node ",
                   sender_index, " for msg ", tag);
    return;
  }

  if (CheckTag(*msg_ptr) && msg_ptr->Id() != id_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", tag, " with sequence ",
                   msg_ptr->SeqCounter(), " and id ", msg_ptr->Id());
    Deliver(broadcasts_[tag].mbar, msg_ptr->Id());
  }
}

void RBC::Deliver(const SerialisedMessage &msg, uint32_t sender_index)
{
  MuddleAddress miner_id{*std::next(current_cabinet_.begin(), sender_index)};
  // TODO: node_.onBroadcast(msg, miner_id);
  // Try to deliver old messages
  std::lock_guard<std::mutex> lock(mutex_deliver_);
  if (!parties_[sender_index].undelivered_msg.empty())
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " checks old tags for node ", sender_index);
    auto old_tag_msg{parties_[sender_index].undelivered_msg.begin()};
    while (old_tag_msg != parties_[sender_index].undelivered_msg.end() and
           old_tag_msg->second.Id() == CHANNEL_BROADCAST and
           old_tag_msg->second.SeqCounter() == parties_[id_].deliver_s)
    {
      // TODO: node_.onBroadcast(broadcasts_[old_tag_msg->second.getTag()].mbar_, miner_id);
      old_tag_msg = parties_[sender_index].undelivered_msg.erase(old_tag_msg);
    }
  }
}

uint32_t RBC::CabinetIndex(const MuddleAddress &other_address) const
{
  return static_cast<uint32_t>(
      std::distance(current_cabinet_.begin(), current_cabinet_.find(other_address)));
}

bool RBC::CheckTag(RBCMessage &msg)
{
  std::lock_guard<std::mutex> lock(mutex_deliver_);
  uint8_t                     seq{parties_[msg.Id()].deliver_s};
  FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " has sequence ", seq, " for node ", msg.id());
  if (msg.ChannelId() == CHANNEL_BROADCAST and msg.SeqCounter() == seq)
  {
    ++parties_[msg.Id()].deliver_s;  // Increase counter
    return true;
  }
  else if (msg.ChannelId() == CHANNEL_BROADCAST and msg.SeqCounter() > seq)
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " has sequence counter ", seq,
                    " does not match tag sequence ", msg.seqCounter(), " for node ", msg.id());
    // Store tag of message for processing later
    if (parties_[msg.Id()].undelivered_msg.find(msg.SeqCounter()) ==
        parties_[msg.SeqCounter()].undelivered_msg.end())
      parties_[msg.Id()].undelivered_msg.insert({msg.SeqCounter(), msg});
  }
  return false;
}

bool RBC::SetPartyFlag(uint32_t l, TagType tag, MsgType m)
{
  std::lock_guard<std::mutex> lock(mutex_flags_);
  auto &                      iter  = parties_[l].flags[tag];
  auto                        index = static_cast<uint32_t>(m);
  if (iter[index])
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " repeated msg type ", msgType_to_string(m),
                    " with tag ", tag);
    return false;
  }
  iter.set(index);
  return true;
}

}  // namespace rbc
}  // namespace dkg
}  // namespace fetch
