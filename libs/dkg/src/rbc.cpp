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

constexpr char const *LOGGING_NAME = "RBC";

/**
 * Helper function to compute truncation of message hash to 8 bytes. Truncation from left side.
 */
TruncatedHash MessageHash(SerialisedMessage const &msg)
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
    return "";
  }
}

/**
 * Creates instance of RBC
 *
 * @param endpoint The muddle endpoint to communicate on
 * @param address The muddle endpoint address
 * @param cabinet Set of muddle addresses that form RBC network
 * @param threshold Threshold number of Byzantine peers
 * @param dkg
 */
RBC::RBC(Endpoint &endpoint, MuddleAddress address, CabinetMembers const &cabinet,
         std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)> call_back)
  : address_{std::move(address)}
  , endpoint_{endpoint}
  , current_cabinet_{cabinet}
  , deliver_msg_callback_{std::move(call_back)}
  , rbc_subscription_(endpoint.Subscribe(SERVICE_DKG, CHANNEL_BROADCAST))
{

  // Set subscription for rbc
  rbc_subscription_->SetMessageHandler([this](MuddleAddress const &from, uint16_t, uint16_t,
                                              uint16_t, muddle::Packet::Payload const &payload,
                                              MuddleAddress) {
    RBCSerializer serialiser(payload);

    // Deserialize the RBCEnvelop
    RBCEnvelop env;
    serialiser >> env;

    // Dispatch the event
    OnRBC(from, env);
  });
}

/**
 * Resets the RBC for a new cabinet
 */
void RBC::ResetCabinet()
{
  assert(!current_cabinet_.empty());
  assert(current_cabinet_.find(address_) != current_cabinet_.end());

  // Set threshold depending on size of committee
  if (current_cabinet_.size() % 3 == 0)
  {
    threshold_ = static_cast<uint32_t>(current_cabinet_.size() / 3 - 1);
  }
  else
  {
    threshold_ = static_cast<uint32_t>(current_cabinet_.size() / 3);
  }
  assert(current_cabinet_.size() > 3 * threshold_);
  std::lock_guard<std::mutex> lock{mutex_broadcast_};
  id_ = static_cast<uint32_t>(
      std::distance(current_cabinet_.begin(), current_cabinet_.find(address_)));
  parties_.clear();
  parties_.resize(current_cabinet_.size());
  broadcasts_.clear();
  msg_counter_ = 0;
}

/**
 * Sends an RBCEnvelop to a particular address
 * @param env RBCEnvelop to be serialised and sent
 * @param address Destination muddle address
 */
void RBC::Send(RBCEnvelop const &env, MuddleAddress const &address)
{
  // Serialise the RBCEnvelop
  RBCSerializerCounter env_counter;
  env_counter << env;

  RBCSerializer env_serializer;
  env_serializer.Reserve(env_counter.size());
  env_serializer << env;

  endpoint_.Send(address, SERVICE_DKG, CHANNEL_BROADCAST, env_serializer.data());
}

/**
 * Broadcasts RBCEnvelop to all subscribed to RBC channel
 *
 * @param env RBCEnvelop message to be serialised and broadcasted
 */
void RBC::Broadcast(RBCEnvelop const &env)
{
  // Serialise the RBCEnvelop
  RBCSerializerCounter env_counter;
  env_counter << env;

  RBCSerializer env_serializer;
  env_serializer.Reserve(env_counter.size());
  env_serializer << env;

  std::lock_guard<std::mutex> lock{mutex_broadcast_};
  endpoint_.Broadcast(SERVICE_DKG, CHANNEL_BROADCAST, env_serializer.data());
}

/**
 * Places a serialised message to be broadcasted via RBC into a RBCEnvelop and broadcasts
 * envelop
 *
 * @param msg Serialised message to be broadcast
 */
void RBC::SendRBroadcast(SerialisedMessage const &msg)
{
  RBCEnvelop env{RBroadcast(CHANNEL_BROADCAST, id_, ++msg_counter_, msg)};
  Broadcast(env);
  OnRBroadcast(std::dynamic_pointer_cast<RBroadcast>(env.Message()), id_);  // Self sending
}

/**
 * Sets the message for a particular tag in broadcasts_
 *
 * @param tag Unique tag of a message sent via RBC
 * @param msg_ptr Shared pointer of the RMessage message (RBCMessage containing a message_)
 * @param sender_index Index of the sender in current_cabinet_
 * @return Bool for whether the value is set
 */
bool RBC::SetMbar(TagType tag, std::shared_ptr<RMessage> msg_ptr, uint32_t sender_index)
{
  std::lock_guard<std::mutex> lock(mutex_broadcast_);
  if (broadcasts_[tag].mbar.empty())
  {
    broadcasts_[tag].mbar = msg_ptr->message();
    return true;
  }
  if (broadcasts_[tag].mbar != msg_ptr->message())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", id_, " received bad r-send message from ", sender_index);
  }
  return false;
}

/**
 * Sets the message hash for a particular tag in broadcasts_
 *
 * @param tag Unique tag of a message sent via RBC
 * @param msg_ptr Shared pointer of the RHash (RBCMesssage containing a hash_)
 * @return Bool for whether the hash contained in RHash equals the MessageHash of message stored in
 * mbar for this tag
 */
bool RBC::SetDbar(TagType tag, std::shared_ptr<RHash> msg_ptr)
{
  std::lock_guard<std::mutex> lock(mutex_broadcast_);
  broadcasts_[tag].dbar = msg_ptr->hash();
  TruncatedHash msg_hash;
  if (!broadcasts_[tag].mbar.empty())
  {
    msg_hash = MessageHash(broadcasts_[tag].mbar);
  }
  return msg_hash == msg_ptr->hash();
}

/**
 * Increments counter for REcho messages
 * @param tag Unique tag of a message sent via RBC
 * @param msg_ptr Shared pointer to REcho message
 * @return Bool for whether we have reached the message count for REcho messages
 */
bool RBC::ReceivedEcho(TagType tag, std::shared_ptr<REcho> msg_ptr)
{
  std::lock_guard<std::mutex> lock(mutex_broadcast_);
  auto &                      msg_count = broadcasts_[tag].msgs_count[msg_ptr->hash()];
  msg_count.echo_count++;
  return (msg_count.echo_count == current_cabinet_.size() - threshold_ &&
          msg_count.ready_count <= threshold_);
}

/**
 * Increments counter for RReady messages
 * @param tag Unique tag of a message sent via RBC
 * @param msg_ptr Shared pointer to RReady message
 * @return MsgCount for tag in broadcasts_
 */
struct RBC::MsgCount RBC::ReceivedReady(TagType tag, std::shared_ptr<RHash> msg_ptr)
{
  std::lock_guard<std::mutex> lock(mutex_broadcast_);
  auto &                      msg_count = broadcasts_[tag].msgs_count[msg_ptr->hash()];
  msg_count.ready_count++;
  MsgCount res = msg_count;
  return res;
}

/**
 * Handler for a new RBCEnvelop message
 *
 * @param from Muddle address of sender
 * @param envelop RBCEnvelop message
 * @param transmitter Muddle address of transmitter
 */
void RBC::OnRBC(MuddleAddress const &from, RBCEnvelop const &envelop)
{
  auto msg_ptr = envelop.Message();
  if (msg_ptr == nullptr)
  {
    return;  // To catch nullptr return in Message() switch statement default
  }
  uint32_t sender_index{CabinetIndex(from)};
  switch (msg_ptr->type())
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

/**
 * Handler for RBroadcast messages. If valid, broadcasts REcho message for this tag
 *
 * @param msg_ptr Shared pointer to RBroadcast message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnRBroadcast(std::shared_ptr<RBroadcast> const msg_ptr, uint32_t sender_index)
{
  TagType tag = msg_ptr->tag();
  if (!SetPartyFlag(sender_index, tag, MsgType::R_SEND))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with counter ", msg_ptr->counter(), " and id ",
                   msg_ptr->id());
    return;
  }
  FETCH_LOG_INFO(LOGGING_NAME, "onRBroadcast: Node ", id_, " received msg ", tag, " from node ",
                 sender_index, " with counter ", msg_ptr->counter(), " and id ", msg_ptr->id());
  if (sender_index == msg_ptr->id())
  {
    if (SetMbar(tag, msg_ptr, sender_index))
    {
      broadcasts_[tag].mbar = msg_ptr->message();
      RBCEnvelop env{REcho{msg_ptr->channel(), msg_ptr->id(), msg_ptr->counter(),
                           MessageHash(msg_ptr->message())}};
      Broadcast(env);
      OnREcho(std::dynamic_pointer_cast<REcho>(env.Message()), id_);  // self sending.
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_,
                   " received wrong r-send msg from node ", sender_index, " for msg ", tag,
                   " with id ", msg_ptr->id());
  }
}

/**
 * Handler for REcho messages. If ReceivedEcho returns true then broadcasts a RReady message for
 * this tag
 *
 * @param msg_ptr Shared pointer to REcho message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnREcho(std::shared_ptr<REcho> const msg_ptr, uint32_t sender_index)
{
  TagType tag = msg_ptr->tag();
  if (!SetPartyFlag(sender_index, tag, MsgType::R_ECHO))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onREcho: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with counter ", msg_ptr->counter(), " and id ",
                   msg_ptr->id());
    return;
  }
  FETCH_LOG_TRACE(LOGGING_NAME, "onREcho: Node ", id_, " received msg ", tag, " from node ",
                  sender_index, " with counter ", msg_ptr->counter(), " and id ", msg_ptr->id());
  if (ReceivedEcho(tag, msg_ptr))
  {
    RBCEnvelop env{RReady{msg_ptr->channel(), msg_ptr->id(), msg_ptr->counter(), msg_ptr->hash()}};
    Broadcast(env);
    OnRReady(std::dynamic_pointer_cast<RReady>(env.Message()), id_);  // self sending.
  }
}

/**
 * Handler for RReady messages. If 2 * threshold_ + 1 RReady messages are received for a particular
 * tag and hash then the message is ready to be delivered. A RRequest message is sent to 2 *
 * threshold_ + 1 peers if SetDBar returns false
 *
 * @param msg_ptr Shared pointer to RReady message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnRReady(std::shared_ptr<RReady> const msg_ptr, uint32_t sender_index)
{
  TagType tag = msg_ptr->tag();
  if (!SetPartyFlag(sender_index, tag, MsgType::R_READY))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRReady: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with counter ", msg_ptr->counter(), " and id ",
                   msg_ptr->id());
    return;
  }
  FETCH_LOG_TRACE(LOGGING_NAME, "onRReady: Node ", id_, " received msg ", tag, " from node ",
                  sender_index, " with counter ", msg_ptr->counter(), " and id ", msg_ptr->id());
  auto msgsCount = ReceivedReady(tag, msg_ptr);
  if (threshold_ > 0 && msgsCount.ready_count == threshold_ + 1 &&
      msgsCount.echo_count < (current_cabinet_.size() - threshold_))
  {
    RBCEnvelop env{RReady{msg_ptr->channel(), msg_ptr->id(), msg_ptr->counter(), msg_ptr->hash()}};
    Broadcast(env);
    OnRReady(std::dynamic_pointer_cast<RReady>(env.Message()), id_);  // self sending.
  }
  else if (threshold_ > 0 && msgsCount.ready_count == 2 * threshold_ + 1)
  {
    broadcasts_[tag].dbar = msg_ptr->hash();

    TruncatedHash msg_hash;
    if (!SetDbar(tag, msg_ptr))
    {
      RBCEnvelop env{RRequest{msg_ptr->channel(), msg_ptr->id(), msg_ptr->counter()}};

      uint32_t counter{0};
      auto     im = current_cabinet_.begin();
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
    else if (CheckTag(*msg_ptr) && msg_ptr->id() != id_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", tag, " with counter ",
                     msg_ptr->counter(), " and id ", msg_ptr->id());
      Deliver(broadcasts_[tag].mbar, msg_ptr->id());
    }
  }
}

/**
 * Handler for RRequest message. If valid and we have the requested message then send a RAnswer
 * message containing message back to sender
 *
 * @param msg_ptr Shared pointer to RRequest message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnRRequest(std::shared_ptr<RRequest> const msg_ptr, uint32_t sender_index)
{
  TagType tag = msg_ptr->tag();
  if (!SetPartyFlag(sender_index, tag, MsgType::R_REQUEST))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRRequest: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with counter ", msg_ptr->counter(), " and id ",
                   msg_ptr->id());
    return;
  }
  FETCH_LOG_TRACE(LOGGING_NAME, "onRRequest: Node ", id_, " received msg ", tag, " from node ",
                  sender_index, " with counter ", msg_ptr->counter(), " and id ", msg_ptr->id());
  if (!broadcasts_[tag].mbar.empty())
  {
    RBCEnvelop env{
        RAnswer{msg_ptr->channel(), msg_ptr->id(), msg_ptr->counter(), broadcasts_[tag].mbar}};

    auto im = std::next(current_cabinet_.begin(), sender_index);
    Send(env, *im);
  }
}

/**
 * Handler for RAnswer messages. If valid, set mbar in broadcasts_ for this tag
 *
 * @param msg_ptr Shared pointer to RAnswer message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnRAnswer(std::shared_ptr<RAnswer> const msg_ptr, uint32_t sender_index)
{
  TagType tag = msg_ptr->tag();
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
  TruncatedHash msg_hash{MessageHash(msg_ptr->message())};
  if (msg_hash == broadcasts_[tag].dbar)
  {
    if (broadcasts_[tag].mbar.empty())
    {
      broadcasts_[tag].mbar = msg_ptr->message();
    }
    else
    {
      // TODO(jmw): Double check this part of protocol
      broadcasts_[tag].mbar = msg_ptr->message();
    }
  }
  else
  {
    // Hash does not match and we received a bad answer
    FETCH_LOG_WARN(LOGGING_NAME, "onRAnswer: Node ", id_, " received bad r-answer from node ",
                   sender_index, " for msg ", tag);
    return;
  }

  if (CheckTag(*msg_ptr) && msg_ptr->id() != id_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", tag, " with counter ",
                   msg_ptr->counter(), " and id ", msg_ptr->id());
    Deliver(broadcasts_[tag].mbar, msg_ptr->id());
  }
}

/**
 * Delivers messages which have reached the end of the protocol
 *
 * @param msg Serialised message which has been reliably broadcasted
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::Deliver(SerialisedMessage const &msg, uint32_t sender_index)
{
  MuddleAddress miner_id{*std::next(current_cabinet_.begin(), sender_index)};
  deliver_msg_callback_(miner_id, msg);
  // Try to deliver old messages
  std::lock_guard<std::mutex> lock(mutex_deliver_);
  if (!parties_[sender_index].undelivered_msg.empty())
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " checks old tags for node ", sender_index);
    auto old_tag_msg = parties_[sender_index].undelivered_msg.begin();
    while (old_tag_msg != parties_[sender_index].undelivered_msg.end() &&
           old_tag_msg->second.id() == CHANNEL_BROADCAST &&
           old_tag_msg->second.counter() == parties_[id_].deliver_s)
    {
      deliver_msg_callback_(miner_id, broadcasts_[old_tag_msg->second.tag()].mbar);
      old_tag_msg = parties_[sender_index].undelivered_msg.erase(old_tag_msg);
    }
  }
}

/**
 * Computes index of an address in current_cabinet_
 *
 * @param other_address Muddle address of cabinet member
 * @return Index of other_address
 */
uint32_t RBC::CabinetIndex(MuddleAddress const &other_address) const
{
  return static_cast<uint32_t>(
      std::distance(current_cabinet_.begin(), current_cabinet_.find(other_address)));
}

/**
 * Checks the channel, and message counter of message is correct for deliver. If counter is
 * incorrect, the message is placed in an undelivered message queue
 *
 * @param msg RBCMessage which is ready to be delivered
 * @return Bool for whether message can be delivered
 */
bool RBC::CheckTag(RBCMessage &msg)
{
  std::lock_guard<std::mutex> lock(mutex_deliver_);
  uint8_t                     msg_counter = parties_[msg.id()].deliver_s;
  FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " has counter ", msg_counter, " for node ", msg.id());
  if (msg.channel() == CHANNEL_BROADCAST && msg.counter() == msg_counter)
  {
    ++parties_[msg.id()].deliver_s;  // Increase counter
    return true;
  }
  else if (msg.channel() == CHANNEL_BROADCAST && msg.counter() > msg_counter)
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " has counter ", msg_counter,
                    " does not match tag counter ", msg.counter(), " for node ", msg.id());
    // Store tag of message for processing later
    if (parties_[msg.id()].undelivered_msg.find(msg.counter()) ==
        parties_[msg.counter()].undelivered_msg.end())
    {
      parties_[msg.id()].undelivered_msg.insert({msg.counter(), msg});
    }
  }
  return false;
}

/**
 * Sets the message flag for the l'th party on receipt of a RBCMessage message type m for
 * tag
 *
 * @param l Index of party for which we received a new RBCMessage
 * @param tag Unique tag of a message sent via RBC
 * @param m Message type of new RBCMessage received
 * @return Bool for whether the message flag for this message type has been set
 */
bool RBC::SetPartyFlag(uint32_t sender_index, TagType tag, MsgType msg_type)
{
  std::lock_guard<std::mutex> lock(mutex_flags_);
  auto &                      iter  = parties_[sender_index].flags[tag];
  auto                        index = static_cast<uint32_t>(msg_type);
  if (iter[index])
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " repeated msg type ", msgType_to_string(m),
                    " with tag ", tag);
    return false;
  }
  iter.set(index);
  return true;
}

}  // namespace dkg
}  // namespace fetch
