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

#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "dkg/rbc_envelop.hpp"
#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::dkg::rbc;

TEST(rbc_messages, broadcast)
{
  RBroadcast broadcast{1, 1, 1, "hello"};

  fetch::serializers::ByteArrayBuffer serialiser{broadcast.Serialize()};

  fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
  RBroadcast                          broadcast1{serialiser1};

  EXPECT_EQ(broadcast1.Message(), broadcast.Message());
  EXPECT_EQ(broadcast1.Tag(), broadcast.Tag());
}

TEST(rbc_messages, echo)
{
  REcho echo{1, 1, 1, "hello"};

  fetch::serializers::ByteArrayBuffer serialiser{echo.Serialize()};

  fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
  REcho                               echo1{serialiser1};

  EXPECT_EQ(echo1.Hash(), echo.Hash());
  EXPECT_EQ(echo1.Tag(), echo.Tag());
}

TEST(rbc_messages, ready)
{
  RReady ready{1, 1, 1, "hello"};

  fetch::serializers::ByteArrayBuffer serialiser{ready.Serialize()};

  fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
  RReady                              ready1{serialiser1};

  EXPECT_EQ(ready1.Hash(), ready.Hash());
  EXPECT_EQ(ready1.Tag(), ready.Tag());
}

TEST(rbc_messages, request)
{
  RRequest request{1, 1, 1};

  fetch::serializers::ByteArrayBuffer serialiser{request.Serialize()};

  fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
  RRequest                            request1{serialiser1};

  EXPECT_EQ(request1.Tag(), request.Tag());
}

TEST(rbc_messages, answer)
{
  RAnswer answer{1, 1, 1, "hello"};

  fetch::serializers::ByteArrayBuffer serialiser{answer.Serialize()};

  fetch::serializers::ByteArrayBuffer serialiser1(serialiser.data());
  RAnswer                             answer1{serialiser1};

  EXPECT_EQ(answer1.Message(), answer.Message());
  EXPECT_EQ(answer1.Tag(), answer.Tag());
}

TEST(rbc_messages, envelope)
{
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
  RBCEnvelop                          env1;
  env_serialiser1 >> env1;

  // Check the message type of envelops match
  EXPECT_EQ(env1.Message()->Type(), RBCMessage::MessageType::RANSWER);
  EXPECT_EQ(env1.Message()->Tag(), answer.Tag());
  EXPECT_EQ(std::dynamic_pointer_cast<RAnswer>(env1.Message())->Message(), answer.Message());
}
