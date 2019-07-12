#include "core/logging.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "dkg/rbc.hpp"

namespace fetch {
namespace dkg {
namespace rbc {

    constexpr char const *LOGGING_NAME   = "RBC";

    // Function to compute truncation of message hash to 8 bytes. Truncation from left side.
    TruncatedHash messageHash(const SerialisedMessage &msg) {
        byte_array::ByteArray msg_hash_256 {crypto::Hash<crypto::SHA256>(msg)};
        return msg_hash_256.SubArray(24);
    }

    std::string RBC::msgType_to_string(MsgType m) {
      switch(m) {
      case MsgType::R_SEND: return "r_send";
      case MsgType::R_ECHO: return "r_echo";
      case MsgType::R_READY: return "r_ready";
      case MsgType::R_REQUEST: return "r_request";
      case MsgType::R_ANSWER: return "r_answer";
      default:
        assert(false);
      }
    }

    RBC::RBC(Endpoint &endpoint, MuddleAddress address, const CabinetMembers &cabinet, uint32_t threshold)
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
        id_ = static_cast<uint32_t>(std::distance(current_cabinet_.begin(), current_cabinet_.find(address_)));
        assert(num_parties_ > 3 * threshold_);
        parties_.resize(num_parties_);

        // set subscription for rbc
        rbc_subscription_->SetMessageHandler([this](MuddleAddress const &from, uint16_t, uint16_t, uint16_t,
                                                    muddle::Packet::Payload const &payload, MuddleAddress transmitter) {

            RBCSerializer serialiser(payload);

            // deserialize the RBCEnvelop
            RBCEnvelop env;
            serialiser >> env;

            // dispatch the event
            onRBC(from, env, transmitter);
        });
    }

    void RBC::send(const RBCEnvelop &env, const MuddleAddress &address) {
        // Serialise the RBCEnvelop
        RBCSerializerCounter env_counter;
        env_counter << env;

        RBCSerializer env_serializer;
        env_serializer.Reserve(env_counter.size());
        env_serializer << env;

        endpoint_.Send(address, SERVICE_DKG, CHANNEL_BROADCAST, env_serializer.data());
    }

    void RBC::broadcast(const RBCEnvelop &env) {
        // Serialise the RBCEnvelop
        RBCSerializerCounter env_counter;
        env_counter << env;

        RBCSerializer env_serializer;
        env_serializer.Reserve(env_counter.size());
        env_serializer << env;

        endpoint_.Broadcast(SERVICE_DKG, CHANNEL_BROADCAST, env_serializer.data());
    }

    void RBC::sendRBroadcast(const SerialisedMessage &msg) {
        RBCEnvelop env{RBroadcast(CHANNEL_BROADCAST, id_, ++seq_, msg)};
        broadcast(env);
        onRBroadcast(std::dynamic_pointer_cast<RBroadcast>(env.getMessage()), id_);
    }

    bool RBC::setMbar(TagType tag, std::shared_ptr<RMessage> msg_ptr, uint32_t senderIndex) {
      std::lock_guard<std::mutex> lock(mutex_broadcast_);
      if (broadcasts_[tag].mbar_.empty()) {
        broadcasts_[tag].mbar_ = msg_ptr->message();
        return true;
      }
      if(broadcasts_[tag].mbar_ != msg_ptr->message()) {
        FETCH_LOG_WARN(LOGGING_NAME, "Node ", id_, " received bad r-send message from ", senderIndex);
      }
      return false;
    }

    bool RBC::setDbar(TagType tag, std::shared_ptr<RHash> msg_ptr) {
      std::lock_guard<std::mutex> lock(mutex_broadcast_);
      broadcasts_[tag].dbar_ = msg_ptr->hash();
      TruncatedHash msg_hash;
      if (!broadcasts_[tag].mbar_.empty()) {
        msg_hash = messageHash(broadcasts_[tag].mbar_);
      }
      return msg_hash == msg_ptr->hash();
    }
    
    bool RBC::receivedEcho(TagType tag, std::shared_ptr<REcho> msg_ptr) {
      std::lock_guard<std::mutex> lock(mutex_broadcast_);
      auto &msgsCount = broadcasts_[tag].msgsCount_[msg_ptr->hash()];
      msgsCount.e_d_++;
      return (msgsCount.e_d_ == current_cabinet_.size() - threshold_ and msgsCount.r_d_ <= threshold_);
    }
    
    struct RBC::MsgCount RBC::receivedReady(TagType tag, std::shared_ptr<RHash> msg_ptr) {
      std::lock_guard<std::mutex> lock(mutex_broadcast_);
      auto &msgsCount = broadcasts_[tag].msgsCount_[msg_ptr->hash()];
      msgsCount.r_d_++;
      MsgCount res = msgsCount;
      return res;
    }

    void RBC::onRBC(MuddleAddress const &from, RBCEnvelop const &envelop, MuddleAddress const &transmitter) {
        auto msg_ptr = envelop.getMessage();
        uint32_t senderIndex {cabinetIndex(from)};
        switch (msg_ptr->getType()) {
            case RBCMessage::MessageType::RBROADCAST:
                FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RBroadcast from node ", senderIndex);
                onRBroadcast(std::dynamic_pointer_cast<RBroadcast>(msg_ptr), senderIndex);
                break;
            case RBCMessage::MessageType::RECHO:
                FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received REcho from node ", senderIndex);
                onREcho(std::dynamic_pointer_cast<REcho>(msg_ptr), senderIndex);
                break;
            case RBCMessage::MessageType::RREADY:
                FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RReady from node ", senderIndex);
                onRReady(std::dynamic_pointer_cast<RReady>(msg_ptr), senderIndex);
                break;
            case RBCMessage::MessageType::RREQUEST:
                FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RRequest from node ", senderIndex);
                onRRequest(std::dynamic_pointer_cast<RRequest>(msg_ptr), senderIndex);
                break;
            case RBCMessage::MessageType::RANSWER:
                FETCH_LOG_TRACE(LOGGING_NAME, "Node: ", id_, " received RAnswer from node ", senderIndex);
                onRAnswer(std::dynamic_pointer_cast<RAnswer>(msg_ptr), senderIndex);
                break;
            default:
                FETCH_LOG_ERROR(LOGGING_NAME, "Node: ", id_, " can not process payload from node ", senderIndex);
        }
    }

    void RBC::onRBroadcast(const std::shared_ptr<RBroadcast> msg_ptr, uint32_t senderIndex) {
        uint64_t tag{msg_ptr->getTag()};
        if(!setPartyFlag(senderIndex, tag, MsgType::R_SEND)) {
            FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_, " received repeated msg ", tag, " from node ",
                    senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
            return;
        }
        FETCH_LOG_INFO(LOGGING_NAME, "onRBroadcast: Node ", id_, " received msg ", tag, " from node ",
                       senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
        if (senderIndex == msg_ptr->id()) {
            if (setMbar(tag, msg_ptr, senderIndex)) {
                broadcasts_[tag].mbar_ = msg_ptr->message();
                RBCEnvelop env{REcho{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter(), messageHash(msg_ptr->message())}};
                broadcast(env);
                onREcho(std::dynamic_pointer_cast<REcho>(env.getMessage()), id_); // self sending.
            } 
        } else {
            FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_, " received wrong r-send msg from node ",
                           senderIndex, " for msg ", tag, " with id ", msg_ptr->id());
        }
    }

    void RBC::onREcho(const std::shared_ptr<REcho> msg_ptr, uint32_t senderIndex) {
        uint64_t tag{msg_ptr->getTag()};
        if (!setPartyFlag(senderIndex, tag, MsgType::R_ECHO)) {
            FETCH_LOG_WARN(LOGGING_NAME, "onREcho: Node ", id_, " received repeated msg ", tag, " from node ",
                           senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
            return;
        }
        FETCH_LOG_INFO(LOGGING_NAME, "onREcho: Node ", id_, " received msg ", tag, " from node ",
                       senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
        if (receivedEcho(tag, msg_ptr)) {
            RBCEnvelop env{RReady{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter(), msg_ptr->hash()}};
            broadcast(env);
            onRReady(std::dynamic_pointer_cast<RReady>(env.getMessage()), id_); // self sending.
        }
    }

    void RBC::onRReady(const std::shared_ptr<RReady> msg_ptr, uint32_t senderIndex) {
        uint64_t tag {msg_ptr->getTag()};
        if (!setPartyFlag(senderIndex, tag, MsgType::R_READY)) {
            FETCH_LOG_WARN(LOGGING_NAME, "onRReady: Node ", id_, " received repeated msg ", tag, " from node ",
                           senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
            return;
        }
        FETCH_LOG_INFO(LOGGING_NAME, "onRReady: Node ", id_, " received msg ", tag, " from node ",
                       senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
        auto msgsCount = receivedReady(tag, msg_ptr);
        if (threshold_ > 0 and msgsCount.r_d_ == threshold_ + 1 and msgsCount.e_d_ < (num_parties_ - threshold_)) {
            RBCEnvelop env{RReady{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter(), msg_ptr->hash()}};
            broadcast(env);
            onRReady(std::dynamic_pointer_cast<RReady>(env.getMessage()), id_); // self sending.
        } else if (threshold_ > 0 and msgsCount.r_d_ == 2 * threshold_ + 1) {
            broadcasts_[tag].dbar_ = msg_ptr->hash();

            TruncatedHash msg_hash;
            if (!setDbar(tag, msg_ptr)) {
                RBCEnvelop env{RRequest{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter()}};

                uint32_t counter {0};
                auto im {current_cabinet_.begin()};
                assert(2 * threshold_ + 1 <= current_cabinet_.size());
                while (counter < 2 * threshold_ + 1) {
                    if (*im != address_) {
                        send(env, *im);
                        ++counter;
                        ++im;
                    }
                }
            } else if (checkTag(*msg_ptr) && msg_ptr->id() != id_) {
                FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", tag, " with sequence ", msg_ptr->seqCounter(), " and id ",
                               msg_ptr->id());
                deliver(broadcasts_[tag].mbar_, msg_ptr->id());
            }
        }
    }

    void RBC::onRRequest(const std::shared_ptr<RRequest> msg_ptr, uint32_t senderIndex) {
        uint64_t tag {msg_ptr->getTag()};
        if (!setPartyFlag(senderIndex, tag, MsgType::R_REQUEST)) {
            FETCH_LOG_WARN(LOGGING_NAME, "onRRequest: Node ", id_, " received repeated msg ", tag, " from node ",
                           senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
            return;
        }
        FETCH_LOG_INFO(LOGGING_NAME, "onRRequest: Node ", id_, " received msg ", tag, " from node ",
                       senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
        if (!broadcasts_[tag].mbar_.empty()) {
            RBCEnvelop env{RAnswer{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter(), broadcasts_[tag].mbar_}};

            auto im = std::next(current_cabinet_.begin(), senderIndex);
            send(env, *im);
        }
    }

    void RBC::onRAnswer(const std::shared_ptr<RAnswer> msg_ptr, uint32_t senderIndex) {
        uint64_t tag {msg_ptr->getTag()};
        if (!setPartyFlag(senderIndex, tag, MsgType::R_ANSWER)) {
            return;
        }
        // If have not set dbar then we did not send a request message
        if (broadcasts_[tag].dbar_ == 0) {
            return;
        }
        //Check the hash of the message
        TruncatedHash msg_hash {messageHash(msg_ptr->message())};
        if (msg_hash == broadcasts_[tag].dbar_) {
            if (broadcasts_[tag].mbar_.empty()) {
                broadcasts_[tag].mbar_ = msg_ptr->message();
            } else {
                // TODO: Double check this part of protocol
                broadcasts_[tag].mbar_ = msg_ptr->message();
            }
        } else {
            // Hash does not match and we received a bad answer
            FETCH_LOG_WARN(LOGGING_NAME, "onRAnswer: Node ", id_, " received bad r-answer from node ",
                           senderIndex, " for msg ", tag);
            return;
        }

        if (checkTag(*msg_ptr) && msg_ptr->id() != id_) {
            FETCH_LOG_INFO(LOGGING_NAME, "Node ", id_, " delivered msg ", tag, " with sequence ", msg_ptr->seqCounter(), " and id ",
                           msg_ptr->id());
            deliver(broadcasts_[tag].mbar_, msg_ptr->id());
        }
    }

    void RBC::deliver(const SerialisedMessage &msg, uint32_t senderIndex) {
        MuddleAddress miner_id{*std::next(current_cabinet_.begin(), senderIndex)};
        //TODO: node_.onBroadcast(msg, miner_id);
        //Try to deliver old messages
        std::lock_guard<std::mutex> lock(mutex_deliver_);
        if (!parties_[senderIndex].undelivered_msg.empty()) {
            FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " checks old tags for node ", senderIndex);
            auto old_tag_msg{parties_[senderIndex].undelivered_msg.begin()};
            while (old_tag_msg != parties_[senderIndex].undelivered_msg.end()
                and old_tag_msg->second.id() == CHANNEL_BROADCAST
                and old_tag_msg->second.seqCounter() == parties_[id_].deliver_s_) {
                //TODO: node_.onBroadcast(broadcasts_[old_tag_msg->second.getTag()].mbar_, miner_id);
                old_tag_msg = parties_[senderIndex].undelivered_msg.erase(old_tag_msg);
            }
        }
    }

    uint32_t RBC::cabinetIndex(const MuddleAddress &other_address) const {
        return static_cast<uint32_t>(std::distance(current_cabinet_.begin(), current_cabinet_.find(other_address)));
    }

    bool RBC::checkTag(RBCMessage &msg) {
        std::lock_guard<std::mutex> lock(mutex_deliver_);
        uint8_t seq {parties_[msg.id()].deliver_s_};
        FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " has sequence ", seq, " for node ", msg.id());
        if (msg.channelId() == CHANNEL_BROADCAST and msg.seqCounter() == seq) {
            ++parties_[msg.id()].deliver_s_; //Increase counter
            return true;
        } else if (msg.channelId() == CHANNEL_BROADCAST and msg.seqCounter() > seq) {
            FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " has sequence counter ", seq, " does not match tag sequence ",
                    msg.seqCounter(), " for node ", msg.id());
            // Store tag of message for processing later
            if (parties_[msg.id()].undelivered_msg.find(msg.seqCounter()) == parties_[msg.seqCounter()].undelivered_msg.end())
                parties_[msg.id()].undelivered_msg.insert({msg.seqCounter(), msg});
        }
        return false;
    }

    bool RBC::setPartyFlag(uint32_t l, TagType tag, MsgType m) {
      std::lock_guard<std::mutex> lock(mutex_flags_);
      auto &iter = parties_[l].flags_[tag];
      auto index = static_cast<uint32_t>(m);
      if(iter[index]) {
        FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " repeated msg type ", msgType_to_string(m),  " with tag ", tag);
        return false;
      }
      iter.set(index);
      return true;
    }

}
}
}
