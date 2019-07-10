#include "core/logging.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "dkg/rbc.hpp"

namespace fetch {
namespace dkg {
namespace rbc {

    constexpr char const *LOGGING_NAME   = "RBC";

    // Function to compute truncation of message hash to 8 bytes. Truncation from left side.
    TruncatedHash messageHash(const std::string &msg) {
        byte_array::ByteArray msg_hash_256 {crypto::Hash<crypto::SHA256>(msg)};
        return msg_hash_256.SubArray(24);
    }


    RBC::RBC(Endpoint &endpoint, ConstByteArray address, uint32_t num_parties)
    : address_{std::move(address)}
    , endpoint_{endpoint}
    , rbc_subscription_(endpoint.Subscribe(SERVICE_DKG, CHANNEL_BROADCAST))
    , id_{cabinetIndex()} //TODO: Should only be called after the cabinet has been reset
    , num_parties_{num_parties}
    {
            threshold_ = num_parties_ / 3; // Maximum number of byzantine parties the protocol can withstand
            parties_.resize(num_parties);

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

    void RBC::sendRBroadcast(const std::string &msg) {

        RBCEnvelop env{RBroadcast(CHANNEL_BROADCAST, id_, ++s, msg)};
        broadcast(env);
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
        if (partyFlag(senderIndex, tag).send_) {
            FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_, " received repeated msg ", tag, " from node ",
                    senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
            return;
        }
        FETCH_LOG_INFO(LOGGING_NAME, "onRBroadcast: Node ", id_, " received msg ", tag, " from node ",
                       senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
        partyFlag(senderIndex, tag).send_ = true;
        if (senderIndex == msg_ptr->id()) {
            if (broadcasts_[tag].mbar_.empty()) {
                broadcasts_[tag].mbar_ = msg_ptr->message();
                RBCEnvelop env{REcho{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter(), messageHash(msg_ptr->message())}};
                broadcast(env);
                onREcho(std::dynamic_pointer_cast<REcho>(env.getMessage()), id_); // self sending.
            } else if (broadcasts_[tag].mbar_ != msg_ptr->message()){
                FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_, " received bad r-send msg from node ",
                               senderIndex, " for msg ", tag);
                return;
            }
        } else {
            FETCH_LOG_WARN(LOGGING_NAME, "onRBroadcast: Node ", id_, " received wrong r-send msg from node ",
                           senderIndex, " for msg ", tag, " with id ", msg_ptr->id());
        }
    }

    void RBC::onREcho(const std::shared_ptr<REcho> msg_ptr, uint32_t senderIndex) {
        uint64_t tag{msg_ptr->getTag()};
        bool echo {partyFlag(senderIndex, tag).echo_};
        if (echo) {
            FETCH_LOG_WARN(LOGGING_NAME, "onREcho: Node ", id_, " received repeated msg ", tag, " from node ",
                           senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
            return;
        }
        partyFlag(senderIndex, tag).echo_ = true;
        FETCH_LOG_INFO(LOGGING_NAME, "onREcho: Node ", id_, " received msg ", tag, " from node ",
                       senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());

        auto &msgsCount = broadcasts_[tag].msgsCount_[msg_ptr->hash()];
        msgsCount.e_d_++;
        if (msgsCount.e_d_ == num_parties_ - threshold_ and
                msgsCount.r_d_ <= threshold_) {
            RBCEnvelop env{RReady{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter(), msg_ptr->hash()}};
            broadcast(env);
            onRReady(std::dynamic_pointer_cast<RReady>(env.getMessage()), id_); // self sending.
        }
    }

    void RBC::onRReady(const std::shared_ptr<RReady> msg_ptr, uint32_t senderIndex) {
        uint64_t tag {msg_ptr->getTag()};
        if (partyFlag(senderIndex, tag).ready_) {
            FETCH_LOG_WARN(LOGGING_NAME, "onRReady: Node ", id_, " received repeated msg ", tag, " from node ",
                           senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
            return;
        }
        partyFlag(senderIndex, tag).ready_ = true;
        FETCH_LOG_INFO(LOGGING_NAME, "onREcho: Node ", id_, " received msg ", tag, " from node ",
                       senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
        auto &msgsCount = broadcasts_[tag].msgsCount_[msg_ptr->hash()];
        msgsCount.r_d_++;

        if (threshold_ > 0 and msgsCount.r_d_ == threshold_ + 1 and msgsCount.e_d_ < (num_parties_ - threshold_)) {
            RBCEnvelop env{RReady{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter(), msg_ptr->hash()}};
            broadcast(env);
            onRReady(std::dynamic_pointer_cast<RReady>(env.getMessage()), id_); // self sending.
        } else if (threshold_ > 0 and msgsCount.r_d_ == 2 * threshold_ + 1) {
            broadcasts_[tag].dbar_ = msg_ptr->hash();

            TruncatedHash msg_hash;
            if (!broadcasts_[tag].mbar_.empty()) {
                msg_hash = messageHash(broadcasts_[tag].mbar_);
            }
            if (msg_hash != msg_ptr->hash()) {
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
        if (partyFlag(senderIndex, tag).request_) {
            FETCH_LOG_WARN(LOGGING_NAME, "onRRequest: Node ", id_, " received repeated msg ", tag, " from node ",
                           senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
            return;
        }
        partyFlag(senderIndex, tag).request_ = true;
        FETCH_LOG_INFO(LOGGING_NAME, "onREcho: Node ", id_, " received msg ", tag, " from node ",
                       senderIndex, " with sequence ", msg_ptr->seqCounter(), " and id ", msg_ptr->id());
        if (!broadcasts_[tag].mbar_.empty()) {
            RBCEnvelop env{RAnswer{msg_ptr->channelId(), msg_ptr->id(), msg_ptr->seqCounter(), broadcasts_[tag].mbar_}};

            auto im = std::next(current_cabinet_.begin(), senderIndex);
            send(env, *im);
        }
    }

    void RBC::onRAnswer(const std::shared_ptr<RAnswer> msg_ptr, uint32_t senderIndex) {
        uint64_t tag {msg_ptr->getTag()};
        if (partyFlag(senderIndex, tag).answer_) {
            return;
        }
        partyFlag(senderIndex, tag).answer_ = true;
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
                // TODO: Not sure about this part
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

    void RBC::deliver(const std::string &msg, uint32_t senderIndex) {
        std::string miner_id{*std::next(current_cabinet_.begin(), senderIndex)};
        //TODO: node_.onBroadcast(msg, miner_id);
        //Try to deliver old messages
        if (!parties_[senderIndex].undelivered_msg.empty()) {
            FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " checks old tags for node ", senderIndex);
            auto old_tag_msg{parties_[senderIndex].undelivered_msg.begin()};
            while (old_tag_msg != parties_[senderIndex].undelivered_msg.end() and checkTag(old_tag_msg->second)) {
                uint64_t old_tag{old_tag_msg->second.getTag()};
                //TODO: node_.onBroadcast(broadcasts_[old_tag].mbar_, miner_id);
                old_tag_msg = parties_[senderIndex].undelivered_msg.erase(old_tag_msg);
            }
        }
    }

    uint32_t RBC::cabinetIndex() const {
        return static_cast<uint32_t>(std::distance(current_cabinet_.begin(), current_cabinet_.find(address_)));
    }

    uint32_t RBC::cabinetIndex(const MuddleAddress &other_address) const {
        return static_cast<uint32_t>(std::distance(current_cabinet_.begin(), current_cabinet_.find(other_address)));
    }

    bool RBC::checkTag(RBCMessage &msg) {
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

    RBC::MsgFlags& RBC::partyFlag(uint32_t senderIndex, TagType tag) {
        if (parties_[senderIndex].flags_.find(tag) == parties_[senderIndex].flags_.end()) {
            FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " create new msgs flags for party ", senderIndex, " tag ", tag);
            parties_[senderIndex].flags_.insert({tag, MsgFlags()});
        } else {
            FETCH_LOG_TRACE(LOGGING_NAME, "Node ", id_, " has msgs flags for party ", senderIndex, " tag ", tag);
        }
        return parties_[senderIndex].flags_.at(tag);
    }
}
}
}