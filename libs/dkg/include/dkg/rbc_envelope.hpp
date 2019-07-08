#pragma once

#include "dkg/rbc_messages.hpp"
#include "core/serializers/byte_array.hpp"
#include "network/muddle/packet.hpp"

namespace fetch {
namespace dkg {

    class RBCEnvelop {
    public:
        using Payload = byte_array::ConstByteArray;

        enum class MessageType {
            RBROADCAST,
            RECHO,
            RREADY,
            RREQUEST,
            RANSWER
        };

        RBCEnvelop() = default;
        explicit RBCEnvelop(MessageType type, const Payload &payload): type_{type}, serialisedMessage_{payload} {};

        template <typename T>
        void serialize(T &serialiser) const {
            serialiser << type_ << serialisedMessage_;
        }
        template <typename T>
        void deserialize(T &serialiser) {
            serialiser >> type_ >> serialisedMessage_;
        }

        uint8_t type() const {
            return static_cast<uint8_t>(type_);
        }
        const Payload& payload() const {
            return serialisedMessage_;
        }


    private:
        MessageType type_;
        Payload serialisedMessage_;

    };

    template<typename T>
    void Serialize(T &serializer, RBCEnvelop::MessageType const &type) {
        serializer << (uint8_t) type;
    }

    template<typename T>
    void Deserialize(T &serializer, RBCEnvelop::MessageType &type) {
        uint8_t val;
        serializer >> val;
        type = (RBCEnvelop::MessageType) val;
    }

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