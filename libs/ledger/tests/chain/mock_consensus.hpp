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

#include "gmock/gmock.h"
#include "ledger/consensus/consensus_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

class MockConsensus : public fetch::ledger::ConsensusInterface
{
public:
  using Block            = fetch::ledger::Block;
  using StorageInterface = fetch::ledger::StorageInterface;

  MOCK_METHOD1(UpdateCurrentBlock, void(Block const &));
  MOCK_METHOD0(GenerateNextBlock, NextBlockPtr());
  MOCK_CONST_METHOD1(ValidBlock, Status(Block const &));

  MOCK_METHOD1(SetMaxCabinetSize, void(uint16_t));
  MOCK_METHOD1(SetBlockInterval, void(uint64_t));
  MOCK_METHOD1(SetAeonPeriod, void(uint16_t));
  MOCK_METHOD1(SetDefaultStartTime, void(uint64_t));

  MOCK_METHOD2(Reset, void(StakeSnapshot const &, StorageInterface &));
  MOCK_METHOD1(Reset, void(StakeSnapshot const &));
  MOCK_METHOD1(SetWhitelist, void(Minerwhitelist const &));
};
