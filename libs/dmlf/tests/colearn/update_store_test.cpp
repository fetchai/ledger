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

#include "core/byte_array/byte_array.hpp"
#include "dmlf/colearn/update_store.hpp"
#include "gtest/gtest.h"

#include <iomanip>

namespace fetch {
namespace dmlf {
namespace colearn {


namespace
{
  using fetch::byte_array::ConstByteArray;

  ConstByteArray a("a");
  ConstByteArray b("b");
  ConstByteArray c("c");
}

TEST(Colearn_UpdateStore, pushPop)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", a, "test");

  auto result = store.GetUpdate("algo", "update");

  EXPECT_EQ(result->update_type, "update");
  EXPECT_EQ(result->data, a);
  EXPECT_EQ(result->source, "test");
}

TEST(Colearn_UpdateStore, pushPushPopPop)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", a, "test");
  store.PushUpdate("algo", "update", b, "test2");

  auto result1 = store.GetUpdate("algo", "update");
  auto result2 = store.GetUpdate("algo", "update");

  EXPECT_EQ(result1->update_type, "update");
  EXPECT_EQ(result1->data, a);
  EXPECT_EQ(result1->source, "test");
  EXPECT_EQ(result2->update_type, "update");
  EXPECT_EQ(result2->data, b);
  EXPECT_EQ(result2->source, "test2");
}

TEST(Colearn_UpdateStore, pushPushPopPushPopPop)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", a, "test");
  store.PushUpdate("algo", "update", b, "test2");
  auto result1 = store.GetUpdate("algo", "update");
  store.PushUpdate("algo", "update", c, "test3");
  auto result2 = store.GetUpdate("algo", "update");
  auto result3 = store.GetUpdate("algo", "update");

  EXPECT_EQ(result1->update_type, "update");
  EXPECT_EQ(result1->data, a);
  EXPECT_EQ(result1->source, "test");
  EXPECT_EQ(result2->update_type, "update");
  EXPECT_EQ(result2->data, b);
  EXPECT_EQ(result2->source, "test2");
  EXPECT_EQ(result3->update_type, "update");
  EXPECT_EQ(result3->data, c);
  EXPECT_EQ(result3->source, "test3");
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
