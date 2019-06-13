#pragma once
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

#include "fake_block_packer.hpp"

#include <gmock/gmock.h>

class MockBlockPacker : public fetch::ledger::BlockPackerInterface
{
public:
  MockBlockPacker()
  {
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(*this, GenerateBlock(_, _, _, _))
        .WillByDefault(Invoke(&fake, &FakeBlockPacker::GenerateBlock));
    ON_CALL(*this, GetBacklog()).WillByDefault(Invoke(&fake, &FakeBlockPacker::GetBacklog));
  }

  /// @name Block Packer Interface
  /// @{
  MOCK_METHOD1(EnqueueTransaction, void(fetch::ledger::TransactionLayout const &));
  MOCK_METHOD1(EnqueueTransaction, void(fetch::ledger::Transaction const &));
  MOCK_METHOD4(GenerateBlock, void(fetch::ledger::Block &, std::size_t, std::size_t,
                                   fetch::ledger::MainChain const &));
  MOCK_CONST_METHOD0(GetBacklog, uint64_t());
  /// @}

  FakeBlockPacker fake;
};
