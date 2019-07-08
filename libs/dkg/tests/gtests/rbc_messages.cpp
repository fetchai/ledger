#include "dkg/rbc_envelope.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::dkg;


TEST(rbc_messages, broadcast) {
    RBroadcast broadcast{1, 1, 1, "hello"};

    EXPECT_EQ(broadcast.message(), "hello");
    EXPECT_EQ(broadcast.channelId(), 1);
    EXPECT_EQ(broadcast.sequenceCounter(), 1);
    EXPECT_EQ(broadcast.nodeId(), 1);

    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> counter;
    counter << broadcast;

    fetch::serializers::ByteArrayBuffer serialiser;
    serialiser.Reserve(counter.size());
    serialiser << broadcast;

    RBroadcast broadcast1;
    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    serialiser1 >> broadcast1;

    EXPECT_EQ(broadcast1.message(), broadcast.message());
    EXPECT_EQ(broadcast1.channelId(), broadcast.channelId());
    EXPECT_EQ(broadcast1.sequenceCounter(), broadcast.sequenceCounter());
    EXPECT_EQ(broadcast1.nodeId(), broadcast.nodeId());
}


TEST(rbc_messages, echo) {
    REcho echo{1,1,1, std::hash<std::string>{}("hello")};

    EXPECT_EQ(echo.hash(), std::hash<std::string>{}("hello"));
    EXPECT_EQ(echo.channelId(), 1);
    EXPECT_EQ(echo.sequenceCounter(), 1);
    EXPECT_EQ(echo.nodeId(), 1);

    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> counter;
    counter << echo;

    fetch::serializers::ByteArrayBuffer serialiser;
    serialiser.Reserve(counter.size());
    serialiser << echo;

    REcho echo1;
    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    serialiser1 >> echo1;

    EXPECT_EQ(echo1.hash(), echo.hash());
    EXPECT_EQ(echo1.channelId(), echo.channelId());
    EXPECT_EQ(echo1.sequenceCounter(), echo.sequenceCounter());
    EXPECT_EQ(echo1.nodeId(), echo.nodeId());
}

TEST(rbc_messages, ready) {
    RReady ready{1,1,1, std::hash<std::string>{}("hello")};

    EXPECT_EQ(ready.hash(), std::hash<std::string>{}("hello"));
    EXPECT_EQ(ready.channelId(), 1);
    EXPECT_EQ(ready.sequenceCounter(), 1);
    EXPECT_EQ(ready.nodeId(), 1);

    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> counter;
    counter << ready;

    fetch::serializers::ByteArrayBuffer serialiser;
    serialiser.Reserve(counter.size());
    serialiser << ready;

    RReady ready1;
    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    serialiser1 >> ready1;

    EXPECT_EQ(ready1.hash(), ready.hash());
    EXPECT_EQ(ready1.channelId(), ready.channelId());
    EXPECT_EQ(ready1.sequenceCounter(), ready.sequenceCounter());
    EXPECT_EQ(ready1.nodeId(), ready.nodeId());
}

TEST(rbc_messages, request) {
    RRequest ready{1,1,1};

    EXPECT_EQ(ready.channelId(), 1);
    EXPECT_EQ(ready.sequenceCounter(), 1);
    EXPECT_EQ(ready.nodeId(), 1);

    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> counter;
    counter << ready;

    fetch::serializers::ByteArrayBuffer serialiser;
    serialiser.Reserve(counter.size());
    serialiser << ready;

    RRequest request1;
    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    serialiser1 >> request1;

    EXPECT_EQ(request1.channelId(), ready.channelId());
    EXPECT_EQ(request1.sequenceCounter(), ready.sequenceCounter());
    EXPECT_EQ(request1.nodeId(), ready.nodeId());
}


TEST(rbc_messages, answer) {
    RAnswer answer{1, 1, 1, "hello"};

    EXPECT_EQ(answer.message(), "hello");
    EXPECT_EQ(answer.channelId(), 1);
    EXPECT_EQ(answer.sequenceCounter(), 1);
    EXPECT_EQ(answer.nodeId(), 1);

    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> counter;
    counter << answer;

    fetch::serializers::ByteArrayBuffer serialiser;
    serialiser.Reserve(counter.size());
    serialiser << answer;

    RAnswer answer1;
    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    serialiser1 >> answer1;

    EXPECT_EQ(answer1.message(), answer.message());
    EXPECT_EQ(answer1.channelId(), answer.channelId());
    EXPECT_EQ(answer1.sequenceCounter(), answer.sequenceCounter());
    EXPECT_EQ(answer1.nodeId(), answer.nodeId());
}

TEST(rbc_messages, envelope) {
    RAnswer answer{1, 1, 1, "hello"};

    EXPECT_EQ(answer.message(), "hello");
    EXPECT_EQ(answer.channelId(), 1);
    EXPECT_EQ(answer.sequenceCounter(), 1);
    EXPECT_EQ(answer.nodeId(), 1);

    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> counter;
    counter << answer;

    fetch::serializers::ByteArrayBuffer serialiser;
    serialiser.Reserve(counter.size());
    serialiser << answer;

    using MessageType = fetch::dkg::RBCEnvelop::MessageType;

    // Put into RBCEnvelop
    RBCEnvelop env{MessageType::RANSWER, serialiser.data()};

    EXPECT_EQ(env.type(), static_cast<uint8_t>(MessageType::RANSWER));

    // Serialise the envelop
    fetch::serializers::SizeCounter<fetch::serializers::ByteArrayBuffer> env_counter;
    env_counter << env;

    fetch::serializers::ByteArrayBuffer env_serialiser;
    env_serialiser.Reserve(env_counter.size());
    env_serialiser << env;

    fetch::serializers::ByteArrayBuffer env_serialiser1{env_serialiser.data()};
    RBCEnvelop env1;
    env_serialiser1 >> env1;

    // Check the message type of envelops match
    EXPECT_EQ(env1.type(), env1.type());

    // Deserialise the payload in envelop
    fetch::serializers::ByteArrayBuffer  serialiser1{env1.payload()};
    RAnswer answer1;
    serialiser1 >> answer1;

    EXPECT_EQ(answer1.message(), answer.message());
    EXPECT_EQ(answer1.channelId(), answer.channelId());
    EXPECT_EQ(answer1.sequenceCounter(), answer.sequenceCounter());
    EXPECT_EQ(answer1.nodeId(), answer.nodeId());
}