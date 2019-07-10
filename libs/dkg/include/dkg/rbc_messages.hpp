#pragma once

#include "crypto/bls_base.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <cstdint>

namespace fetch {
namespace dkg {
namespace rbc {

    using TruncatedHash = byte_array::ByteArray;
    using TagType = uint64_t;
    using RBCSerializer  = fetch::serializers::ByteArrayBuffer;
    using RBCSerializerCounter = fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer>;

    class RBCMessage {
    public:
        enum class MessageType : uint8_t {
            RBROADCAST,
            RECHO,
            RREADY,
            RREQUEST,
            RANSWER
        };

        TagType getTag() const {
            TagType msg_tag = channel_id;
            msg_tag <<= 48;
            msg_tag |= node_id;
            msg_tag <<= 32;
            return (msg_tag | uint64_t(sequence_counter));
        }
        RBCMessage::MessageType getType() const {
            return type_;
        }
        uint8_t channelId() const {
            return channel_id;
        }
        uint8_t seqCounter() const {
            return sequence_counter;
        }
        uint32_t id() const {
            return node_id;
        }
        virtual RBCSerializer serialize() const = 0;

    protected:
        const MessageType type_;
        // Tag of message
        uint16_t channel_id{0}; // Is this safe to truncate to uint8_t?
        uint32_t node_id{0}; // Should this be crypto::bls::Id?
        uint8_t sequence_counter{0};

        explicit RBCMessage(MessageType type): type_{type} {};
        explicit RBCMessage(uint16_t channel, uint32_t node, uint8_t seq, MessageType type) : type_{type}, channel_id{channel}, node_id{node},
            sequence_counter{seq} {};
    };

    class RMessage : public RBCMessage {
    public:
        RBCSerializer serialize() const override {
            RBCSerializer serialiser;
            serialiser << channel_id << node_id << sequence_counter << message_;
            return serialiser;
        }

        // For tests:
        const std::string& message() const {
            return message_;
        }
    protected:
        std::string message_;
        explicit RMessage(MessageType type): RBCMessage{type} {};
        RMessage(uint16_t channel, uint32_t node, uint8_t seq, std::string msg, MessageType type): RBCMessage{channel, node,
                seq, type}, message_{std::move(msg)} {};
    };

    class RHash : public RBCMessage {
    public:
        RBCSerializer serialize() const override {
            RBCSerializer serialiser;
            serialiser << channel_id << node_id << sequence_counter << hash_;
            return serialiser;
        }

        // For tests:
        const TruncatedHash& hash() const {
            return hash_;
        }
    protected:
        TruncatedHash hash_;
        explicit RHash(MessageType type): RBCMessage{type} {};
        RHash(uint16_t channel, uint32_t node, uint8_t seq, TruncatedHash msg_hash, MessageType type): RBCMessage{channel, node,
                                                                                            seq, type}, hash_{msg_hash} {};
    };

    // Different RBC Messages:

    // Create this message for messages that need to be broadcasted
    class RBroadcast : public RMessage {
    public:
        explicit RBroadcast(uint16_t channel, uint32_t node, uint8_t seq_counter, std::string msg):
            RMessage{channel, node, seq_counter, std::move(msg), MessageType::RBROADCAST} {};
        explicit RBroadcast(RBCSerializer &serialiser): RMessage{MessageType::RBROADCAST} {
            serialiser >> channel_id >> node_id >> sequence_counter >> message_;
        }
    };

    // Send an REcho when we receive an RBroadcast message
    class REcho : public RHash {
    public:
        explicit REcho(uint16_t channel, uint32_t node, uint8_t seq_counter, TruncatedHash msg_hash):
                RHash{channel, node, seq_counter, msg_hash, MessageType::RECHO} {};
        explicit REcho(RBCSerializer &serialiser): RHash{MessageType::RECHO} {
            serialiser >> channel_id >> node_id >> sequence_counter >> hash_;
        }

    };

    // RReady messages which are sent once we receive num_parties - threshold REcho's
    class RReady : public RHash {
    public:
        explicit RReady(uint16_t channel, uint32_t node, uint8_t seq_counter, TruncatedHash msg_hash):
                RHash{channel, node, seq_counter, msg_hash, MessageType::RREADY} {};
        RReady(RBCSerializer &serialiser): RHash{MessageType::RREADY} {
            serialiser >> channel_id >> node_id >> sequence_counter >> hash_;
        }
    };

    // Make RRequest if the hash of RReady messages does not match our RBroadcast message
    class RRequest : public RBCMessage {
    public:
        explicit RRequest(uint16_t channel, uint32_t node, uint8_t seq_counter):
        RBCMessage{channel, node, seq_counter, MessageType::RREQUEST} {};
        explicit RRequest(RBCSerializer &serialiser): RBCMessage{MessageType::RREQUEST} {
            serialiser >> channel_id >> node_id >> sequence_counter;
        }

        RBCSerializer serialize() const override {
            RBCSerializer serialiser;
            serialiser << channel_id << node_id << sequence_counter;
            return serialiser;
        }
    };

    // Reply to RRequest messages
    class RAnswer : public RMessage {
    public:
        explicit RAnswer(uint16_t channel, uint32_t node, uint8_t seq_counter, std::string msg):
                RMessage{channel, node, seq_counter, std::move(msg), MessageType::RANSWER} {};
        explicit RAnswer(RBCSerializer &serialiser): RMessage{MessageType::RANSWER} {
            serialiser >> channel_id >> node_id >> sequence_counter >> message_;
        }
    };
}
}
}