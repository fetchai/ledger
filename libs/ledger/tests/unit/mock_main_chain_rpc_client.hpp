#pragma once
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

#include "ledger/protocols/main_chain_rpc_client_interface.hpp"
#include "muddle/create_muddle_fake.hpp"

#include "gmock/gmock.h"

class MockMainChainRpcClient : public fetch::ledger::MainChainRpcClientInterface
{
public:
  using Digest = fetch::Digest;

  MOCK_METHOD2(GetHeaviestChain, BlocksPromise(MuddleAddress, uint64_t));
  MOCK_METHOD4(GetCommonSubChain, BlocksPromise(MuddleAddress, Digest, Digest, uint64_t));
  MOCK_METHOD2(TimeTravel, TraveloguePromise(MuddleAddress, Digest));
};
