//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "muddle/rbc_messages.hpp"

#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::muddle;

TEST(rbc_messages, broadcast)
{
  RBroadcast broadcast{1, 1, 1, "hello"};

  fetch::serializers::MsgPackSerializer serialiser{broadcast.Serialize()};

  fetch::serializers::MsgPackSerializer serialiser1(serialiser.data());
  RBCMessage                            broadcast1;
  serialiser1 >> broadcast1;

  EXPECT_EQ(broadcast1.message(), broadcast.message());
  EXPECT_EQ(broadcast1.tag(), broadcast.tag());
}

TEST(rbc_messages, echo)
{
  REcho echo{1, 1, 1, "hello"};

  fetch::serializers::MsgPackSerializer serialiser{echo.Serialize()};

  fetch::serializers::MsgPackSerializer serialiser1(serialiser.data());
  RBCMessage                            echo1;
  serialiser1 >> echo1;

  EXPECT_EQ(echo1.hash(), echo.hash());
  EXPECT_EQ(echo1.tag(), echo.tag());
}

TEST(rbc_messages, ready)
{
  RReady ready{1, 1, 1, "hello"};

  fetch::serializers::MsgPackSerializer serialiser{ready.Serialize()};

  fetch::serializers::MsgPackSerializer serialiser1(serialiser.data());
  RBCMessage                            ready1;
  serialiser1 >> ready1;

  EXPECT_EQ(ready1.hash(), ready.hash());
  EXPECT_EQ(ready1.tag(), ready.tag());
}

TEST(rbc_messages, request)
{
  RRequest request{1, 1, 1};

  fetch::serializers::MsgPackSerializer serialiser{request.Serialize()};

  fetch::serializers::MsgPackSerializer serialiser1(serialiser.data());
  RBCMessage                            request1;
  serialiser1 >> request1;

  EXPECT_EQ(request1.tag(), request.tag());
}

TEST(rbc_messages, answer)
{
  RAnswer answer{1, 1, 1, "hello"};

  fetch::serializers::MsgPackSerializer serialiser{answer.Serialize()};

  fetch::serializers::MsgPackSerializer serialiser1(serialiser.data());
  RBCMessage                            answer1;
  serialiser1 >> answer1;

  EXPECT_EQ(answer1.message(), answer.message());
  EXPECT_EQ(answer1.tag(), answer.tag());
}
