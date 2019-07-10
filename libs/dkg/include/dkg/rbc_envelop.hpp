#pragma once

#include "dkg/rbc_messages.hpp"
#include "core/serializers/byte_array.hpp"
#include "network/muddle/packet.hpp"

namespace fetch {
namespace dkg {
namespace rbc {

    class RBCEnvelop {
        using MessageType = RBCMessage::MessageType;
        using Payload = byte_array::ConstByteArray;
    public:
        RBCEnvelop() = default;

        explicit RBCEnvelop(const RBCMessage &msg) : type_{msg.getType()},
                                                     serialisedMessage_{msg.serialize().data()} {};

        template<typename T>
        void serialize(T &serialiser) const {
            serialiser << (uint8_t) type_ << serialisedMessage_;
        }

        template<typename T>
        void deserialize(T &serialiser) {
            uint8_t val;
            serialiser >> val;
            type_ = (MessageType) val;
            serialiser >> serialisedMessage_;
        }

        std::shared_ptr<RBCMessage> getMessage() const {
            RBCSerializer serialiser{serialisedMessage_};
            switch (type_) {
                case MessageType::RBROADCAST:
                    return std::make_shared<RBroadcast>(serialiser);
                case MessageType::RECHO:
                    return std::make_shared<REcho>(serialiser);
                case MessageType::RREADY:
                    return std::make_shared<RReady>(serialiser);
                case MessageType::RREQUEST:
                    return std::make_shared<RRequest>(serialiser);
                case MessageType::RANSWER:
                    return std::make_shared<RAnswer>(serialiser);
            }
        }

    private:
        RBCMessage::MessageType type_;
        Payload serialisedMessage_;
    };

    template<typename T>
    inline void Serialize(T &serializer, RBCEnvelop const &env) {
        env.serialize(serializer);
    }

    template<typename T>
    inline void Deserialize(T &serializer, RBCEnvelop &env) {
        env.deserialize(serializer);
    }
}
}
}