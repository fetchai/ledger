#include "dkg/rbc_envelope.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::dkg;


TEST(rbc_messages, broadcast) {
    RBroadcast broadcast{1, 1, 1, "hello"};

    fetch::serializers::ByteArrayBuffer serialiser {broadcast.serialize()};

    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    RBroadcast broadcast1{serialiser1};

    EXPECT_EQ(broadcast1.message(), broadcast.message());
    EXPECT_EQ(broadcast1.getTag(), broadcast.getTag());
}


TEST(rbc_messages, echo) {
    REcho echo{1,1,1, std::hash<std::string>{}("hello")};

    fetch::serializers::ByteArrayBuffer serialiser {echo.serialize()};

    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    REcho echo1{serialiser1};

    EXPECT_EQ(echo1.hash(), echo.hash());
    EXPECT_EQ(echo1.getTag(), echo.getTag());
}

TEST(rbc_messages, ready) {
    RReady ready{1,1,1, std::hash<std::string>{}("hello")};

    fetch::serializers::ByteArrayBuffer serialiser {ready.serialize()};

    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    RReady ready1{serialiser1};

    EXPECT_EQ(ready1.hash(), ready.hash());
    EXPECT_EQ(ready1.getTag(), ready.getTag());
}

TEST(rbc_messages, request) {
    RRequest request{1,1,1};

    fetch::serializers::ByteArrayBuffer serialiser {request.serialize()};

    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    RRequest request1{serialiser1};

    EXPECT_EQ(request1.getTag(), request.getTag());
}


TEST(rbc_messages, answer) {
    RAnswer answer{1, 1, 1, "hello"};

    fetch::serializers::ByteArrayBuffer serialiser {answer.serialize()};

    fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
    RAnswer answer1{serialiser1};

    EXPECT_EQ(answer1.message(), answer.message());
    EXPECT_EQ(answer1.getTag(), answer.getTag());
}

TEST(rbc_messages, envelope) {
    RAnswer answer{1, 1, 1, "hello"};

    // Put into RBCEnvelop
    RBCEnvelop env{answer};

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
    EXPECT_EQ(env1.getMessage()->getType(), RBCMessage::MessageType::RANSWER);
    EXPECT_EQ(env1.getMessage()->getTag(), answer.getTag());
    EXPECT_EQ(std::dynamic_pointer_cast<RAnswer>(env1.getMessage()) -> message(), answer.message());
}