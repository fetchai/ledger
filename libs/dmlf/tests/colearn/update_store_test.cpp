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
#include "dmlf/colearn/colearn_uri.hpp"
#include "dmlf/colearn/update_store.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <thread>

namespace fetch {
namespace dmlf {
namespace colearn {

namespace {
using fetch::byte_array::ConstByteArray;

ConstByteArray a("a");
ConstByteArray b("b");
ConstByteArray c("c");

const std::string consumer  = "consumer";
const std::string consumerb = "consumerb";

using UpdatePtr = UpdateStore::UpdatePtr;

auto LifoCriteria = [](UpdatePtr const &u) {
  return static_cast<double>(-u->TimeSinceCreation().count());
};
auto FifoCriteria = [](UpdatePtr const &u) {
  return static_cast<double>(u->TimeSinceCreation().count());
};

}  // namespace

TEST(Colearn_UpdateStore, pushPop)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});

  auto result = store.GetUpdate("algo", "update");

  EXPECT_EQ(result->update_type(), "update");
  EXPECT_EQ(result->data(), a);
  EXPECT_EQ(result->source(), "test");
}

TEST(Colearn_UpdateStore, pushPushPopPop)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test2", {});

  auto result1 = store.GetUpdate("algo", "update", consumer);
  auto result2 = store.GetUpdate("algo", "update", consumer);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), b);
  EXPECT_EQ(result1->source(), "test2");
  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), a);
  EXPECT_EQ(result2->source(), "test");
}

TEST(Colearn_UpdateStore, pushPushPopPushPopPop)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test2", {});
  auto result1 = store.GetUpdate("algo", "update", consumer);
  store.PushUpdate("algo", "update", ConstByteArray{c}, "test3", {});
  auto result2 = store.GetUpdate("algo", "update", consumer);
  auto result3 = store.GetUpdate("algo", "update", consumer);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), b);
  EXPECT_EQ(result1->source(), "test2");
  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), c);
  EXPECT_EQ(result2->source(), "test3");
  EXPECT_EQ(result3->update_type(), "update");
  EXPECT_EQ(result3->data(), a);
  EXPECT_EQ(result3->source(), "test");
}

TEST(Colearn_UpdateStore, pushPushPopPushPopPop_TwoConsumersSameCriteria)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test2", {});
  auto result1  = store.GetUpdate("algo", "update", consumer);
  auto result1b = store.GetUpdate("algo", "update", consumerb);
  store.PushUpdate("algo", "update", ConstByteArray{c}, "test3", {});
  auto result2  = store.GetUpdate("algo", "update", consumer);
  auto result2b = store.GetUpdate("algo", "update", consumerb);
  auto result3  = store.GetUpdate("algo", "update", consumer);
  auto result3b = store.GetUpdate("algo", "update", consumerb);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), b);
  EXPECT_EQ(result1->source(), "test2");
  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), c);
  EXPECT_EQ(result2->source(), "test3");
  EXPECT_EQ(result3->update_type(), "update");
  EXPECT_EQ(result3->data(), a);
  EXPECT_EQ(result3->source(), "test");

  EXPECT_EQ(result1b->update_type(), "update");
  EXPECT_EQ(result1b->data(), b);
  EXPECT_EQ(result1b->source(), "test2");
  EXPECT_EQ(result2b->update_type(), "update");
  EXPECT_EQ(result2b->data(), c);
  EXPECT_EQ(result2b->source(), "test3");
  EXPECT_EQ(result3b->update_type(), "update");
  EXPECT_EQ(result3b->data(), a);
  EXPECT_EQ(result3b->source(), "test");
}

TEST(Colearn_UpdateStore, URI_pushPushPopPushPopPop_TwoConsumersSameCriteria)
{
  UpdateStore store;

  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("test"),
                   ConstByteArray{a}, {});
  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("test2"),
                   ConstByteArray{b}, {});
  auto result1 =
      store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"), consumer);
  auto result1b =
      store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"), consumerb);
  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("test3"),
                   ConstByteArray{c}, {});
  auto result2 =
      store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"), consumer);
  auto result2b =
      store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"), consumerb);
  auto result3 =
      store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"), consumer);
  auto result3b =
      store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"), consumerb);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), b);
  EXPECT_EQ(result1->source(), "test2");
  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), c);
  EXPECT_EQ(result2->source(), "test3");
  EXPECT_EQ(result3->update_type(), "update");
  EXPECT_EQ(result3->data(), a);
  EXPECT_EQ(result3->source(), "test");

  EXPECT_EQ(result1b->update_type(), "update");
  EXPECT_EQ(result1b->data(), b);
  EXPECT_EQ(result1b->source(), "test2");
  EXPECT_EQ(result2b->update_type(), "update");
  EXPECT_EQ(result2b->data(), c);
  EXPECT_EQ(result2b->source(), "test3");
  EXPECT_EQ(result3b->update_type(), "update");
  EXPECT_EQ(result3b->data(), a);
  EXPECT_EQ(result3b->source(), "test");
}

TEST(Colearn_UpdateStore, pushPushPopPushPopPop_TwoConsumersDiffCriteria)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test2", {});
  auto result1  = store.GetUpdate("algo", "update", LifoCriteria, consumer);
  auto result1b = store.GetUpdate("algo", "update", FifoCriteria, consumerb);
  store.PushUpdate("algo", "update", ConstByteArray{c}, "test3", {});
  auto result2  = store.GetUpdate("algo", "update", LifoCriteria, consumer);
  auto result2b = store.GetUpdate("algo", "update", FifoCriteria, consumerb);
  auto result3  = store.GetUpdate("algo", "update", LifoCriteria, consumer);
  auto result3b = store.GetUpdate("algo", "update", FifoCriteria, consumerb);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), b);
  EXPECT_EQ(result1->source(), "test2");
  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), c);
  EXPECT_EQ(result2->source(), "test3");
  EXPECT_EQ(result3->update_type(), "update");
  EXPECT_EQ(result3->data(), a);
  EXPECT_EQ(result3->source(), "test");

  EXPECT_EQ(result1b->update_type(), "update");
  EXPECT_EQ(result1b->data(), a);
  EXPECT_EQ(result1b->source(), "test");
  EXPECT_EQ(result2b->update_type(), "update");
  EXPECT_EQ(result2b->data(), b);
  EXPECT_EQ(result2b->source(), "test2");
  EXPECT_EQ(result3b->update_type(), "update");
  EXPECT_EQ(result3b->data(), c);
  EXPECT_EQ(result3b->source(), "test3");
}

TEST(Colearn_UpdateStore, pushPushPopPushPopPop_NoConsumer)
{
  const std::string noConsumer;

  UpdateStore store;
  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test2", {});
  auto result1 = store.GetUpdate("algo", "update", noConsumer);
  store.PushUpdate("algo", "update", ConstByteArray{c}, "test3", {});
  auto result2 = store.GetUpdate("algo", "update", noConsumer);
  auto result3 = store.GetUpdate("algo", "update", noConsumer);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), b);
  EXPECT_EQ(result1->source(), "test2");
  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), c);
  EXPECT_EQ(result2->source(), "test3");
  EXPECT_EQ(result3->update_type(), "update");
  EXPECT_EQ(result3->data(), c);
  EXPECT_EQ(result3->source(), "test3");
}

TEST(Colearn_UpdateStore, pushPop_repetition)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  EXPECT_EQ(store.GetUpdateCount(), 1);
  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  EXPECT_EQ(store.GetUpdateCount(), 1);

  auto result = store.GetUpdate("algo", "update", consumer);
  EXPECT_EQ(store.GetUpdateCount(), 1);

  EXPECT_EQ(result->update_type(), "update");
  EXPECT_EQ(result->data(), a);
  EXPECT_EQ(result->source(), "test");

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  EXPECT_EQ(store.GetUpdateCount(), 1);

  EXPECT_THROW(store.GetUpdate("algo", "update", consumer), std::runtime_error);
}

TEST(Colearn_UpdateStore, samePushDifferentSources)
{
  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  EXPECT_EQ(store.GetUpdateCount(), 1);
  store.PushUpdate("algo", "update", ConstByteArray{a}, "other", {});
  EXPECT_EQ(store.GetUpdateCount(), 2);

  auto result = store.GetUpdate("algo", "update", consumer);
  EXPECT_EQ(store.GetUpdateCount(), 2);

  EXPECT_EQ(result->update_type(), "update");
  EXPECT_EQ(result->data(), a);
  EXPECT_EQ(result->source(), "other");

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  EXPECT_EQ(store.GetUpdateCount(), 2);
  store.PushUpdate("algo", "update", ConstByteArray{a}, "other", {});
  EXPECT_EQ(store.GetUpdateCount(), 2);

  result = store.GetUpdate("algo", "update", consumer);
  EXPECT_EQ(result->update_type(), "update");
  EXPECT_EQ(result->data(), a);
  EXPECT_EQ(result->source(), "test");
}

TEST(Colearn_UpdateStore, pushPushPushPopPopPop_SelectSource)
{
  auto LifoSelect = [](UpdatePtr const &update) -> double {
    if (update->source() != "thinker")
    {
      return std::nan("");
    }
    return static_cast<double>(-update->TimeSinceCreation().count());
  };

  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test2", {});
  store.PushUpdate("algo", "update", ConstByteArray{a}, "thinker", {});

  auto result1 = store.GetUpdate("algo", "update", LifoSelect, consumer);
  auto resulta = store.GetUpdate("algo", "update", consumerb);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), a);
  EXPECT_EQ(result1->source(), "thinker");
  EXPECT_EQ(resulta->update_type(), "update");
  EXPECT_EQ(resulta->data(), a);
  EXPECT_EQ(resulta->source(), "thinker");

  store.PushUpdate("algo", "update", ConstByteArray{b}, "thinker", {});
  store.PushUpdate("algo", "update", ConstByteArray{c}, "thinker", {});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test", {});

  auto result2 = store.GetUpdate("algo", "update", LifoSelect, consumer);
  auto result3 = store.GetUpdate("algo", "update", LifoSelect, consumer);
  auto resultb = store.GetUpdate("algo", "update", LifoCriteria, consumerb);

  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), c);
  EXPECT_EQ(result2->source(), "thinker");
  EXPECT_EQ(result3->update_type(), "update");
  EXPECT_EQ(result3->data(), b);
  EXPECT_EQ(result3->source(), "thinker");
  EXPECT_EQ(resultb->update_type(), "update");
  EXPECT_EQ(resultb->data(), b);
  EXPECT_EQ(resultb->source(), "test");

  EXPECT_THROW(store.GetUpdate("algo", "update", LifoSelect, consumer), std::runtime_error);
  auto result4 = store.GetUpdate("algo", "update", LifoCriteria, consumer);
  EXPECT_EQ(result4->update_type(), "update");
  EXPECT_EQ(result4->data(), b);
  EXPECT_EQ(result4->source(), "test");

  auto resultc = store.GetUpdate("algo", "update", LifoSelect, consumerb);
  EXPECT_EQ(resultc->update_type(), "update");
  EXPECT_EQ(resultc->data(), c);
  EXPECT_EQ(resultc->source(), "thinker");
}

TEST(Colearn_UpdateStore, pushPushPushPopPopPop_SelectMetadata)
{
  std::string which;
  auto        LifoSelect = [&which](UpdatePtr const &update) -> double {
    if (update->metadata().at("meta") != which)
    {
      return std::nan("");
    }
    return static_cast<double>(-update->TimeSinceCreation().count());
  };

  UpdateStore store;

  store.PushUpdate("algo", "update", ConstByteArray{a}, "test", {{"meta", "a"}});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test2", {{"meta", "b"}});
  store.PushUpdate("algo", "update", ConstByteArray{a}, "thinker", {{"meta", "c"}});

  which        = "a";
  auto result1 = store.GetUpdate("algo", "update", LifoSelect, consumer);
  auto resulta = store.GetUpdate("algo", "update", consumerb);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), a);
  EXPECT_EQ(result1->source(), "test");
  EXPECT_EQ(resulta->update_type(), "update");
  EXPECT_EQ(resulta->data(), a);
  EXPECT_EQ(resulta->source(), "thinker");

  store.PushUpdate("algo", "update", ConstByteArray{b}, "thinker", {{"meta", "d"}});
  store.PushUpdate("algo", "update", ConstByteArray{c}, "thinker", {{"meta", "e"}});
  store.PushUpdate("algo", "update", ConstByteArray{b}, "test", {{"meta", "f"}});

  which        = "c";
  auto result2 = store.GetUpdate("algo", "update", LifoSelect, consumer);
  which        = "b";
  auto result3 = store.GetUpdate("algo", "update", LifoSelect, consumer);
  auto resultb = store.GetUpdate("algo", "update", LifoCriteria, consumerb);

  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), a);
  EXPECT_EQ(result2->source(), "thinker");
  EXPECT_EQ(result3->update_type(), "update");
  EXPECT_EQ(result3->data(), b);
  EXPECT_EQ(result3->source(), "test2");
  EXPECT_EQ(resultb->update_type(), "update");
  EXPECT_EQ(resultb->data(), b);
  EXPECT_EQ(resultb->source(), "test");

  EXPECT_THROW(store.GetUpdate("algo", "update", LifoSelect, consumer), std::runtime_error);
  which        = "d";
  auto result4 = store.GetUpdate("algo", "update", LifoSelect, consumer);
  EXPECT_EQ(result4->update_type(), "update");
  EXPECT_EQ(result4->data(), b);
  EXPECT_EQ(result4->source(), "thinker");

  which = "f";
  EXPECT_THROW(store.GetUpdate("algo", "update", LifoSelect, consumerb), std::runtime_error);
  which        = "a";
  auto resultc = store.GetUpdate("algo", "update", LifoSelect, consumerb);
  EXPECT_EQ(resultc->update_type(), "update");
  EXPECT_EQ(resultc->data(), a);
  EXPECT_EQ(resultc->source(), "test");
}

TEST(Colearn_UpdateStore, URI_pushPushPushPopPopPop_SelectMetadata)
{
  std::string which;
  auto        LifoSelect = [&which](UpdatePtr const &update) -> double {
    if (update->metadata().at("meta") != which)
    {
      return std::nan("");
    }
    return static_cast<double>(-update->TimeSinceCreation().count());
  };

  UpdateStore store;

  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("test"),
                   ConstByteArray{a}, {{"meta", "a"}});
  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("test2"),
                   ConstByteArray{b}, {{"meta", "b"}});
  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("thinker"),
                   ConstByteArray{a}, {{"meta", "c"}});

  which        = "a";
  auto result1 = store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"),
                                 LifoSelect, consumer);
  auto resulta =
      store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"), consumerb);

  EXPECT_EQ(result1->update_type(), "update");
  EXPECT_EQ(result1->data(), a);
  EXPECT_EQ(result1->source(), "test");
  EXPECT_EQ(resulta->update_type(), "update");
  EXPECT_EQ(resulta->data(), a);
  EXPECT_EQ(resulta->source(), "thinker");

  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("thinker"),
                   ConstByteArray{b}, {{"meta", "d"}});
  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("thinker"),
                   ConstByteArray{c}, {{"meta", "e"}});
  store.PushUpdate(ColearnURI().algorithm_class("algo").update_type("update").source("test"),
                   ConstByteArray{b}, {{"meta", "f"}});

  which        = "c";
  auto result2 = store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"),
                                 LifoSelect, consumer);
  which        = "b";
  auto result3 = store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"),
                                 LifoSelect, consumer);
  auto resultb = store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"),
                                 LifoCriteria, consumerb);

  EXPECT_EQ(result2->update_type(), "update");
  EXPECT_EQ(result2->data(), a);
  EXPECT_EQ(result2->source(), "thinker");
  EXPECT_EQ(result3->update_type(), "update");
  EXPECT_EQ(result3->data(), b);
  EXPECT_EQ(result3->source(), "test2");
  EXPECT_EQ(resultb->update_type(), "update");
  EXPECT_EQ(resultb->data(), b);
  EXPECT_EQ(resultb->source(), "test");

  EXPECT_THROW(store.GetUpdate("algo", "update", LifoSelect, consumer), std::runtime_error);
  which        = "d";
  auto result4 = store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"),
                                 LifoSelect, consumer);
  EXPECT_EQ(result4->update_type(), "update");
  EXPECT_EQ(result4->data(), b);
  EXPECT_EQ(result4->source(), "thinker");

  which = "f";
  EXPECT_THROW(store.GetUpdate("algo", "update", LifoSelect, consumerb), std::runtime_error);
  which        = "a";
  auto resultc = store.GetUpdate(ColearnURI().algorithm_class("algo").update_type("update"),
                                 LifoSelect, consumerb);
  EXPECT_EQ(resultc->update_type(), "update");
  EXPECT_EQ(resultc->data(), a);
  EXPECT_EQ(resultc->source(), "test");
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
