#pragma once

#include "crypto/bls_base.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/serializers/stl_types.hpp"

#include <cstdint>

namespace fetch {
namespace dkg {

    using TruncatedHash = uint64_t;
    using TagType = uint64_t;

    class RBCMessage {
    protected:
        // Tag of message
        uint16_t channel_id{0}; // Is this safe to truncate to uint8_t?
        uint32_t node_id{0}; // Should this be crypto::bls::Id?
        uint8_t sequence_counter{0};

        RBCMessage() = default;

        explicit RBCMessage(uint16_t channel, uint32_t node, uint8_t seq) : channel_id{channel}, node_id{node},
            sequence_counter{seq} {};

    public:
        TagType getTag() {
            TagType msg_tag = channel_id;
            msg_tag <<= 48;
            msg_tag |= node_id;
            msg_tag <<= 32;
            return (msg_tag | uint64_t(sequence_counter));
        }

        uint16_t channelId() const {
            return channel_id;
        }
        uint8_t sequenceCounter() const {
            return sequence_counter;
        }
        uint32_t nodeId() const {
            return node_id;
        }
    };

    class RMessage : public RBCMessage {
    protected:
        std::string message_;
        RMessage() = default;
        RMessage(uint16_t channel, uint32_t node, uint8_t seq, std::string msg): RBCMessage{channel, node,
                seq}, message_{std::move(msg)} {};
    public:
        template <typename T>
        void serialize(T &serialiser) const {
            serialiser << channel_id << node_id << sequence_counter << message_;
        }
        template <typename T>
        void deserialize(T &serialiser) {
            serialiser >> channel_id >> node_id >> sequence_counter >> message_;
        }

        const std::string& message() const {
            return message_;
        }
    };

    class RHash : public RBCMessage {
    protected:
        TruncatedHash hash_;
        RHash() = default;
        RHash(uint16_t channel, uint32_t node, uint8_t seq, TruncatedHash msg_hash): RBCMessage{channel, node,
                                                                                            seq}, hash_{msg_hash} {};
    public:
        template <typename T>
        void serialize(T &serialiser) const {
            serialiser << channel_id << node_id << sequence_counter << hash_;
        }
        template <typename T>
        void deserialize(T &serialiser) {
            serialiser >> channel_id >> node_id >> sequence_counter >> hash_;
        }

        const TruncatedHash& hash() const {
            return hash_;
        }
    };

    // Different RBC Messages:

    // Create this message for messages that need to be broadcasted
    class RBroadcast : public RMessage {
    public:

        RBroadcast() = default;
        explicit RBroadcast(uint16_t channel, uint32_t node, uint8_t seq_counter, std::string msg):
            RMessage{channel, node, seq_counter, std::move(msg)} {};
    };

    template<typename T>
    inline void Serialize(T &serializer, RBroadcast const &broadcast) {
        broadcast.serialize(serializer);
    }

    template<typename T>
    inline void Deserialize(T &serializer, RBroadcast &broadcast) {
        broadcast.deserialize(serializer);
    }

    // Send an REcho when we receive an RBroadcast message
    class REcho : public RHash {
    public:

        REcho() = default;
        explicit REcho(uint16_t channel, uint32_t node, uint8_t seq_counter, TruncatedHash msg_hash):
                RHash{channel, node, seq_counter, msg_hash} {};
    };

    template<typename T>
    inline void Serialize(T &serializer, REcho const &echo) {
        echo.serialize(serializer);
    }

    template<typename T>
    inline void Deserialize(T &serializer, REcho &echo) {
        echo.deserialize(serializer);
    }

    // RReady messages which are sent once we receive num_parties - threshold REcho's
    class RReady : public RHash {
    public:

        RReady() = default;
        explicit RReady(uint16_t channel, uint32_t node, uint8_t seq_counter, TruncatedHash msg_hash):
                RHash{channel, node, seq_counter, msg_hash} {};
    };

    template<typename T>
    inline void Serialize(T &serializer, RReady const &ready) {
        ready.serialize(serializer);
    }

    template<typename T>
    inline void Deserialize(T &serializer, RReady &ready) {
        ready.deserialize(serializer);
    }

    // Make RRequest if the hash of RReady messages does not match our RBroadcast message
    class RRequest : public RBCMessage {
    public:
        RRequest() = default;
        explicit RRequest(uint16_t channel, uint32_t node, uint8_t seq_counter):
        RBCMessage{channel, node, seq_counter} {};

        template <typename T>
        void serialize(T &serialiser) const {
            serialiser << channel_id << node_id << sequence_counter;
        }
        template <typename T>
        void deserialize(T &serialiser) {
            serialiser >> channel_id >> node_id >> sequence_counter;
        }
    };

    template<typename T>
    inline void Serialize(T &serializer, RRequest const &request) {
        request.serialize(serializer);
    }

    template<typename T>
    inline void Deserialize(T &serializer, RRequest &request) {
        request.deserialize(serializer);
    }


    // Reply to RRequest messages
    class RAnswer : public RMessage {
    public:

        RAnswer() = default;
        explicit RAnswer(uint16_t channel, uint32_t node, uint8_t seq_counter, std::string msg):
                RMessage{channel, node, seq_counter, std::move(msg)} {};
    };

    template<typename T>
    inline void Serialize(T &serializer, RAnswer const &answer) {
        answer.serialize(serializer);
    }

    template<typename T>
    inline void Deserialize(T &serializer, RAnswer &answer) {
        answer.deserialize(serializer);
    }
}
}