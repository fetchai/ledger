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
 * Creates instance of RBC
 *
 * @param endpoint The muddle endpoint to communicate on
 * @param address The muddle endpoint address
 * @param cabinet Set of muddle addresses that form RBC network
 * @param threshold Threshold number of Byzantine peers
 * @param dkg
 */
RBC::RBC(Endpoint &endpoint, MuddleAddress address, CallbackFunction call_back, uint16_t channel)
  : channel_{channel}
  , address_{std::move(address)}
  , endpoint_{endpoint}
  , deliver_msg_callback_{std::move(call_back)}
  , rbc_subscription_(endpoint.Subscribe(SERVICE_DKG, channel_))
{

  // Set subscription for rbc
  rbc_subscription_->SetMessageHandler([this](MuddleAddress const &from, uint16_t, uint16_t,
                                              uint16_t, muddle::Packet::Payload const &payload,
                                              MuddleAddress) {
    RBCSerializer serialiser(payload);

    // Deserialize the RBCEnvelope
    RBCMessage msg;
    try
    {
      serialiser >> msg;
    }
    catch (...)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Node ", id_,
                      " caught an exception while deserializing RBC message. ");

      // Deserialization failed - bad payload.
      return;
    }

    // Dispatch the event
    OnRBC(from, msg);
  });
}

/**
 * Resets the RBC for a new cabinet
 */
bool RBC::ResetCabinet(CabinetMembers const &cabinet)
{
  assert(!lock_.owns_lock());
  FETCH_LOCK(lock_);

  // Empty cabinets cannot be instated.
  if (cabinet.empty())
  {
    return false;
  }

  // Copying cabinet members
  current_cabinet_ = cabinet;

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

  auto iterator = current_cabinet_.find(address_);

  // Cannot determine id_ if not present in the
  // current cabinet.
  if (iterator == current_cabinet_.end())
  {
    return false;
  }

  // Finding own id.
  id_ = static_cast<uint32_t>(std::distance(current_cabinet_.begin(), iterator));

  // Reseting the state of the RBC
  parties_.clear();
  parties_.resize(current_cabinet_.size());
  broadcasts_.clear();
  msg_counter_ = 0;

  return true;
}

/**
 * Places a serialised message to be broadcast via RBC into an RBCEnvelope and broadcasts
 * the envelope
 *
 * @param msg Serialised message to be broadcast
 */
void RBC::SendRBroadcast(SerialisedMessage const &msg)
{
  MessageBroadcast broadcast_msg;

  {  // Creating broadcast message and broadcasting
    assert(!lock_.owns_lock());
    FETCH_LOCK(lock_);

    broadcast_msg = RBCMessage::New<RBroadcast>(channel_, static_cast<IdType>(id_),
                                                static_cast<CounterType>(++msg_counter_), msg);
    Broadcast(*broadcast_msg);
  }

  // Sending message to self
  OnRBroadcast(broadcast_msg, id_);
}

/**
 * Sends an RBCEnvelope to a particular address
 * @param env RBCEnvelope to be serialised and sent
 * @param address Destination muddle address
 */
void RBC::Send(RBCMessage const &msg, MuddleAddress const &address)
{
  // Serialise the RBCEnvelope
  RBCSerializerCounter msg_counter;
  msg_counter << msg;

  RBCSerializer msg_serializer;
  msg_serializer.Reserve(msg_counter.size());
  msg_serializer << msg;

  endpoint_.Send(address, SERVICE_DKG, channel_, msg_serializer.data());
}

/**
 * Broadcasts RBCEnvelope to all subscribed to RBC channel
 *
 * @param env RBCEnvelope message to be serialised and broadcast
 */
void RBC::Broadcast(RBCMessage const &msg)
{
  // Serialise the RBCEnvelope
  RBCSerializerCounter msg_counter;
  msg_counter << msg;

  RBCSerializer msg_serializer;
  msg_serializer.Reserve(msg_counter.size());
  msg_serializer << msg;

  // Broadcast without echo
  assert(lock_.owns_lock());
  for (const auto &address : current_cabinet_)
  {
    if (address != address_)
    {
      endpoint_.Send(address, SERVICE_DKG, channel_, msg_serializer.data());
    }
  }
}

/**
 * Increments counter for REcho messages
 * @param tag Unique tag of a message sent via RBC
 * @param msg Reference to REcho message
 * @return Bool for whether we have reached the message count for REcho messages
 */
bool RBC::ReceivedEcho(TagType tag, MessageEcho const &msg)
{
  assert(lock_.owns_lock());

  auto &msg_count = broadcasts_[tag].msgs_count[msg->hash()];
  msg_count.echo_count++;
  return (msg_count.echo_count == current_cabinet_.size() - threshold_ &&
          msg_count.ready_count <= threshold_);
}

/**
 * Increments counter for RReady messages
 * @param tag Unique tag of a message sent via RBC
 * @param msg Reference to RReady message
 * @return MessageCount for tag in broadcasts_
 */
struct RBC::MessageCount RBC::ReceivedReady(TagType tag, MessageXHash const &msg)
{
  assert(lock_.owns_lock());

  auto &msg_count = broadcasts_[tag].msgs_count[msg->hash()];
  msg_count.ready_count++;
  MessageCount res = msg_count;
  return res;
}

/**
 * Handler for a new RBCEnvelope message
 *
 * @param from Muddle address of sender
 * @param envelope RBCEnvelope message
 * @param transmitter Muddle address of transmitter
 */
void RBC::OnRBC(MuddleAddress const &from, RBCMessage const &message)
{
  assert(!lock_.owns_lock());

  uint32_t sender_index;

  {
    FETCH_LOCK(lock_);
    sender_index = CabinetIndex(from);

    if (!BasicMessageCheck(from, message))
    {
      return;
    }
  }

  switch (message.type())
  {
  case RBCMessageType::R_BROADCAST:
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RBroadcast from node ", sender_index);
    OnRBroadcast(RBCMessage::New<RBroadcast>(message), sender_index);
    break;
  }
  case RBCMessageType::R_ECHO:
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received REcho from node ", sender_index);
    OnREcho(RBCMessage::New<REcho>(message), sender_index);
    break;
  }
  case RBCMessageType::R_READY:
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RReady from node ", sender_index);
    OnRReady(RBCMessage::New<RReady>(message), sender_index);
    break;
  }
  case RBCMessageType::R_REQUEST:
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RRequest from node ", sender_index);
    OnRRequest(RBCMessage::New<RRequest>(message), sender_index);
    break;
  }
  case RBCMessageType::R_ANSWER:
  {
    FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RAnswer from node ", sender_index);
    OnRAnswer(RBCMessage::New<RAnswer>(message), sender_index);
    break;
  }
  default:
    FETCH_LOG_WARN(LOGGING_NAME, "Node: ", id_, " can not process payload from node ",
                   sender_index);
  }
}

/**
 * Handler for RBroadcast messages. If valid, broadcasts REcho message for this tag
 *
 * @param msg Reference to RBroadcast message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnRBroadcast(MessageBroadcast const &msg, uint32_t sender_index)
{
  assert(!lock_.owns_lock());
  FETCH_LOCK(lock_);
  assert(msg != nullptr);
  assert(msg->type() == MessageType::R_BROADCAST);
  assert(msg->is_valid());
  TagType tag = msg->tag();

  if (!SetPartyFlag(sender_index, tag, MessageType::R_BROADCAST))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with counter ", msg->counter(), " and id ",
                   msg->id());
    return;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "onRBroadcast: Node ", id_, " received msg ", tag, " from node ",
                 sender_index, " with counter ", msg->counter(), " and id ", msg->id());
  if (sender_index == msg->id())
  {
    if (SetMbar(tag, msg, sender_index))
    {

      MessageEcho echo_msg = RBCMessage::New<REcho>(msg->channel(), msg->id(), msg->counter(),
                                                    crypto::Hash<HashFunction>(msg->message()));
      Broadcast(*echo_msg);
      OnREchoLockFree(echo_msg, id_);
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_,
                   " received wrong r-send msg from node ", sender_index, " for msg ", tag,
                   " with id ", msg->id());
  }
}

/**
 * Handler for REcho messages. If ReceivedEcho returns true then broadcasts a RReady message for
 * this tag
 *
 * @param msg Reference to REcho message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnREcho(MessageEcho const &msg, uint32_t sender_index)
{
  assert(!lock_.owns_lock());
  FETCH_LOCK(lock_);
  assert(msg != nullptr);
  assert(msg->is_valid());

  OnREchoLockFree(msg, sender_index);
}

void RBC::OnREchoLockFree(MessageEcho const &msg, uint32_t sender_index)
{
  assert(lock_.owns_lock());
  assert(msg != nullptr);
  assert(msg->is_valid());
  TagType tag = msg->tag();
  if (!SetPartyFlag(sender_index, tag, MessageType::R_ECHO))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onREcho: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with counter ", msg->counter(), " and id ",
                   msg->id());
    return;
  }

  FETCH_LOG_TRACE(LOGGING_NAME, "onREcho: Node ", id_, " received msg ", tag, " from node ",
                  sender_index, " with counter ", msg->counter(), " and id ", msg->id());
  if (ReceivedEcho(tag, msg))
  {
    MessageReady ready_msg =
        RBCMessage::New<RReady>(msg->channel(), msg->id(), msg->counter(), msg->hash());

    Broadcast(*ready_msg);
    OnRReadyLockFree(ready_msg, id_);
  }
}

/**
 * Handler for RReady messages. If 2 * threshold_ + 1 RReady messages are received for a particular
 * tag and hash then the message is ready to be delivered. A RRequest message is sent to 2 *
 * threshold_ + 1 peers if SetDBar returns false
 *
 * @param msg Reference to RReady message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnRReady(MessageReady const &msg, uint32_t sender_index)
{
  assert(!lock_.owns_lock());
  FETCH_LOCK(lock_);

  OnRReadyLockFree(msg, sender_index);
}

void RBC::OnRReadyLockFree(MessageReady const &msg, uint32_t sender_index)
{
  assert(lock_.owns_lock());
  assert(msg != nullptr);
  assert(msg->is_valid());

  TagType tag = msg->tag();

  if (!SetPartyFlag(sender_index, tag, MessageType::R_READY))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRReady: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with counter ", msg->counter(), " and id ",
                   msg->id());
    return;
  }

  FETCH_LOG_TRACE(LOGGING_NAME, "onRReady: Node ", id_, " received msg ", tag, " from node ",
                  sender_index, " with counter ", msg->counter(), " and id ", msg->id());
  auto msgs_counter = ReceivedReady(tag, msg);

  if (threshold_ > 0 && msgs_counter.ready_count == threshold_ + 1 &&
      msgs_counter.echo_count < (current_cabinet_.size() - threshold_))
  {
    MessageReady ready_msg =
        RBCMessage::New<RReady>(msg->channel(), msg->id(), msg->counter(), msg->hash());

    Broadcast(*ready_msg);
    OnRReadyLockFree(ready_msg, id_);
  }
  else if (msgs_counter.ready_count == 2 * threshold_ + 1)
  {
    if (!SetDbar(tag, msg))
    {
      MessageRequest send_msg =
          RBCMessage::New<RRequest>(msg->channel(), msg->id(), msg->counter());

      uint32_t counter{0};
      auto     im = current_cabinet_.begin();
      assert(2 * threshold_ + 1 <= current_cabinet_.size());

      while (counter < 2 * threshold_ + 1 && im != current_cabinet_.end())
      {
        if (*im != address_)
        {
          Send(*send_msg, *im);
          ++counter;
        }
        ++im;
      }
    }
    else if (msg->id() != id_ && CheckTag(*msg))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", tag, " with counter ",
                     msg->counter(), " and id ", msg->id());

      SerialisedMessage message_to_send = broadcasts_[tag].original_message;

      Deliver(message_to_send, msg->id());

      delivered_.insert(tag);
    }
  }
  else
  {
    if (msgs_counter.ready_count == current_cabinet_.size() - 1 &&
        delivered_.find(tag) != delivered_.end())
    {  // all messages arrived let's clean

      broadcasts_.erase(tag);
      parties_[sender_index].flags.erase(tag);
    }
  }
}  // namespace dkg

/**
 * Handler for RRequest message. If valid and we have the requested message then send a RAnswer
 * message containing message back to sender
 *
 * @param msg Reference to RRequest message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnRRequest(MessageRequest const &msg, uint32_t sender_index)
{
  assert(!lock_.owns_lock());
  FETCH_LOCK(lock_);
  assert(msg != nullptr);
  assert(msg->is_valid());

  TagType tag = msg->tag();
  if (!SetPartyFlag(sender_index, tag, MessageType::R_REQUEST))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "onRRequest: Node ", id_, " received repeated msg ", tag,
                   " from node ", sender_index, " with counter ", msg->counter(), " and id ",
                   msg->id());
    return;
  }
  FETCH_LOG_TRACE(LOGGING_NAME, "onRRequest: Node ", id_, " received msg ", tag, " from node ",
                  sender_index, " with counter ", msg->counter(), " and id ", msg->id());
  if (!broadcasts_[tag].original_message.empty())
  {
    MessageAnswer nmsg = RBCMessage::New<RAnswer>(msg->channel(), msg->id(), msg->counter(),
                                                  broadcasts_[tag].original_message);

    auto im = std::next(current_cabinet_.begin(), sender_index);
    Send(*nmsg, *im);
  }
}

/**
 * Handler for RAnswer messages. If valid, set original_message in broadcasts_ for this tag
 *
 * @param msg Reference to RAnswer message
 * @param sender_index Index of sender in current_cabinet_
 */
void RBC::OnRAnswer(MessageAnswer const &msg, uint32_t sender_index)
{
  assert(!lock_.owns_lock());
  FETCH_LOCK(lock_);
  assert(msg != nullptr);
  assert(msg->is_valid());

  TagType tag = msg->tag();
  if (!SetPartyFlag(sender_index, tag, MessageType::R_ANSWER))
  {
    return;
  }
  // If have not set message_hash then we did not send a request message
  if (broadcasts_[tag].message_hash.empty())
  {
    return;
  }
  // Check the hash of the message
  MessageHash msg_hash{crypto::Hash<HashFunction>(msg->message())};

  if (msg_hash == broadcasts_[tag].message_hash)
  {
    if (broadcasts_[tag].original_message.empty())
    {
      broadcasts_[tag].original_message = msg->message();
    }
    else
    {
      // TODO(jmw): Double check this part of protocol
      broadcasts_[tag].original_message = msg->message();
    }
  }
  else
  {
    // Hash does not match and we received a bad answer
    FETCH_LOG_WARN(LOGGING_NAME, "onRAnswer: Node ", id_, " received bad r-answer from node ",
                   sender_index, " for msg ", tag);
    return;
  }

  if (msg->id() != id_ && CheckTag(*msg))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", tag, " with counter ",
                   msg->counter(), " and id ", msg->id());

    SerialisedMessage message_to_send = broadcasts_[tag].original_message;

    Deliver(message_to_send, msg->id());

    delivered_.insert(tag);
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
  assert(lock_.owns_lock());

  assert(parties_.size() == current_cabinet_.size());

  MuddleAddress miner_id{*std::next(current_cabinet_.begin(), sender_index)};
  deliver_msg_callback_(miner_id, msg);
  ++parties_[sender_index].deliver_s;  // Increase counter

  // Try to deliver old messages
  if (!parties_[sender_index].undelivered_msg.empty())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " checks old messages for node ", sender_index);
    auto old_tag_msg = parties_[sender_index].undelivered_msg.begin();

    while (old_tag_msg != parties_[sender_index].undelivered_msg.end() &&
           old_tag_msg->first == parties_[sender_index].deliver_s)
    {
      TagType old_tag = old_tag_msg->second;
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, "testing msg with tag ", old_tag);
      assert(!broadcasts_[old_tag].original_message.empty());
      FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", old_tag, " with counter ",
                     old_tag_msg->first, " and id ", sender_index);
      deliver_msg_callback_(miner_id, broadcasts_[old_tag].original_message);
      ++parties_[sender_index].deliver_s;  // Increase counter
      delivered_.insert(old_tag);
      old_tag_msg = parties_[sender_index].undelivered_msg.erase(old_tag_msg);
    }
  }
}

/**
 * Helper function to check basic details of the message to determine if it should be processed
 *
 * @param from Muddle address of sender
 * @param msg_ptr Shared pointer of message
 * @return Bool of whether the message passes the test or not
 */
bool RBC::BasicMessageCheck(MuddleAddress const &from, RBCMessage const &msg)
{
  assert(lock_.owns_lock());

  if ((current_cabinet_.find(from) == current_cabinet_.end()) || (msg.channel() != channel_))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", id_,
                   " received message from unknown sender/wrong channel");
    return false;
  }

  return true;
}

/**
 * Sets the message for a particular tag in broadcasts_
 *
 * @param tag Unique tag of a message sent via RBC
 * @param msg Reference of the RMessage message (RBCMessage containing a message_)
 * @param sender_index Index of the sender in current_cabinet_
 * @return Bool for whether the value is set
 */
bool RBC::SetMbar(TagType tag, MessageContents const &msg, uint32_t sender_index)
{
  assert(lock_.owns_lock());
  assert(msg != nullptr);
  assert(msg->is_valid());

  if (broadcasts_[tag].original_message.empty())
  {
    broadcasts_[tag].original_message = msg->message();
    return true;
  }

  if (broadcasts_[tag].original_message != msg->message())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", id_, " received bad r-send message from ", sender_index);
  }
  return false;
}

/**
 * Sets the message hash for a particular tag in broadcasts_
 *
 * @param tag Unique tag of a message sent via RBC
 * @param msg Reference of the RHash (RBCMesssage containing a hash_)
 * @return Bool for whether the hash contained in RHash equals the MessageHash of message stored in
 * original_message for this tag
 */
bool RBC::SetDbar(TagType tag, MessageXHash const &msg)
{
  assert(lock_.owns_lock());
  broadcasts_[tag].message_hash = msg->hash();
  assert(msg != nullptr);
  assert(msg->is_valid());

  MessageHash msg_hash;

  if (!broadcasts_[tag].original_message.empty())
  {
    msg_hash = crypto::Hash<HashFunction>(broadcasts_[tag].original_message);
  }
  return msg_hash == msg->hash();
}

/**
 * Computes index of an address in current_cabinet_
 *
 * @param other_address Muddle address of cabinet member
 * @return Index of other_address
 */
uint32_t RBC::CabinetIndex(MuddleAddress const &other_address) const
{
  assert(lock_.owns_lock());

  auto iter = current_cabinet_.find(other_address);
  assert(iter != current_cabinet_.end());
  return static_cast<uint32_t>(std::distance(current_cabinet_.begin(), iter));
}

/**
 * Checks the channel, and message counter of message is correct for deliver. If counter is
 * incorrect, the message is placed in an undelivered message queue
 *
 * @param msg RBCMessage which is ready to be delivered
 * @return Bool for whether message can be delivered
 */
bool RBC::CheckTag(RBCMessage const &msg)
{
  assert(lock_.owns_lock());

  if (msg.id() >= current_cabinet_.size())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", id_, " received message with unknown tag id");
    return false;
  }
  assert(parties_.size() == current_cabinet_.size());

  uint8_t msg_counter = parties_[msg.id()].deliver_s;

  FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " has counter ", msg_counter, " for node ", msg.id());
  assert(msg.channel() == channel_);
  if (msg.counter() == msg_counter)
  {
    return true;
  }
  else if (msg.counter() > msg_counter)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Node ", id_, " has counter ", msg_counter,
                   " does not match tag counter ", msg.counter(), " for node ", msg.id());
    // Store counter of message for processing later
    if (parties_[msg.id()].undelivered_msg.find(msg.counter()) ==
        parties_[msg.id()].undelivered_msg.end())
    {
      parties_[msg.id()].undelivered_msg.insert({msg.counter(), msg.tag()});
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
bool RBC::SetPartyFlag(uint32_t sender_index, TagType tag, MessageType msg_type)
{
  assert(lock_.owns_lock());
  assert(parties_.size() == current_cabinet_.size());

  auto &iter  = parties_[sender_index].flags[tag];
  auto  index = static_cast<uint32_t>(msg_type);
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
