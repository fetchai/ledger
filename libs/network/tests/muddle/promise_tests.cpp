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

#include "network/muddle/peer_list.hpp"
#include "network/muddle/router.hpp"
#include "network/service/promise.hpp"

#include <gmock/gmock.h>
#include <memory>

class PeerConnectionListTests : public ::testing::Test
{
protected:
  using Router                = fetch::muddle::Router;
  using RouterPtr             = std::unique_ptr<Router>;
  using PeerConnectionList    = fetch::muddle::PeerConnectionList;
  using PeerConnectionListPtr = std::unique_ptr<PeerConnectionList>;

  void SetUp() override
  {
    //    router_ = std::make_unique<Router>();
    //    peer_list_ = std::make_unique<PeerConnectionList>();
  }

  RouterPtr             router_;
  PeerConnectionListPtr peer_list_;
};

TEST(PromiseTests, CheckNormalPromiseCycle)
{
  auto prom = fetch::service::MakePromise();

  bool success  = false;
  bool failure  = false;
  bool complete = false;

  prom->WithHandlers()
      .Then([&success]() { success = true; })
      .Catch([&failure]() { failure = true; })
      .Finally([&complete]() { complete = true; });

  EXPECT_FALSE(success);
  EXPECT_FALSE(failure);
  EXPECT_FALSE(complete);

  prom->Fulfill(fetch::byte_array::ConstByteArray{});

  EXPECT_TRUE(success);
  EXPECT_FALSE(failure);
  EXPECT_TRUE(complete);
}

TEST(PromiseTests, CheckNormalFailureCycle)
{
  auto prom = fetch::service::MakePromise();

  bool success  = false;
  bool failure  = false;
  bool complete = false;

  prom->WithHandlers()
      .Then([&success]() { success = true; })
      .Catch([&failure]() { failure = true; })
      .Finally([&complete]() { complete = true; });

  EXPECT_FALSE(success);
  EXPECT_FALSE(failure);
  EXPECT_FALSE(complete);

  prom->Fail();

  EXPECT_FALSE(success);
  EXPECT_TRUE(failure);
  EXPECT_TRUE(complete);
}

TEST(PromiseTests, CheckImmediateSuccess)
{
  auto prom = fetch::service::MakePromise();

  bool success  = false;
  bool failure  = false;
  bool complete = false;

  prom->Fulfill(fetch::byte_array::ConstByteArray{});

  EXPECT_FALSE(success);
  EXPECT_FALSE(failure);
  EXPECT_FALSE(complete);

  prom->WithHandlers()
      .Then([&success]() { success = true; })
      .Catch([&failure]() { failure = true; })
      .Finally([&complete]() { complete = true; });

  EXPECT_TRUE(success);
  EXPECT_FALSE(failure);
  EXPECT_TRUE(complete);
}

TEST(PromiseTests, CheckImmediateFailure)
{
  auto prom = fetch::service::MakePromise();

  bool success  = false;
  bool failure  = false;
  bool complete = false;

  prom->Fail();

  EXPECT_FALSE(success);
  EXPECT_FALSE(failure);
  EXPECT_FALSE(complete);

  prom->WithHandlers()
      .Then([&success]() { success = true; })
      .Catch([&failure]() { failure = true; })
      .Finally([&complete]() { complete = true; });

  EXPECT_FALSE(success);
  EXPECT_TRUE(failure);
  EXPECT_TRUE(complete);
}
