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

#include "ledger/dag/dag.hpp"
#include "network/service/promise.hpp"
#include "network/service/protocol.hpp"

namespace fetch {
namespace ledger {

class DAGSyncProtocol : public fetch::service::Protocol
{
public:

  enum
  {
    REQUEST_NODES          = 1,
  };

  static constexpr char const *LOGGING_NAME = "DAGSyncProtocol";

  // Construction / Destruction
  explicit DAGSyncProtocol(std::shared_ptr<ledger::DAG> dag);
  DAGSyncProtocol(DAGSyncProtocol const &) = delete;
  DAGSyncProtocol(DAGSyncProtocol &&)      = delete;
  ~DAGSyncProtocol() override              = default;

  // Operators
  DAGSyncProtocol &operator = (DAGSyncProtocol const &) = delete;
  DAGSyncProtocol &operator = (DAGSyncProtocol &&) = delete;

private:

  using Self    = DAGSyncProtocol;

  using MissingTXs             = DAG::MissingTXs;
  using MissingNodes           = DAG::MissingNodes;

  DAG::MissingNodes RequestNodes(MissingTXs);

  std::shared_ptr<ledger::DAG>  dag_;
};

}  // namespace ledger
}  // namespace fetch

