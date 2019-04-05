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

#include "ledger/block_sink_interface.hpp"

#include <deque>

class FakeBlockSink : public fetch::ledger::BlockSinkInterface
{
public:
  using BlockQueue = std::deque<fetch::ledger::Block>;

  /// @name Block Sink Interface
  /// @{
  void OnBlock(fetch::ledger::Block const &block) override;
  /// @}

  BlockQueue const &queue();
  void              clear();

private:
  BlockQueue block_queue_{};
};