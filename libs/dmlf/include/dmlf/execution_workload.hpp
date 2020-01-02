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

#include "core/byte_array/const_byte_array.hpp"
#include "dmlf/execution/execution_engine_interface.hpp"
#include "dmlf/execution/execution_interface.hpp"
#include "dmlf/execution/execution_result.hpp"

namespace fetch {
namespace dmlf {

class ExecutionWorkload
{
  friend class RemoteExecutionHost;

public:
  using ExecutionEngineInterfacePtr = std::shared_ptr<ExecutionEngineInterface>;
  using Respondent                  = fetch::byte_array::ConstByteArray;
  using OpIdent                     = std::string;
  using Worker                      = std::function<ExecutionResult(ExecutionEngineInterfacePtr)>;
  using Name                        = ExecutionInterface::Name;

  ExecutionWorkload(Respondent respondent, OpIdent op_id, Name state_name, Worker worker)
    : respondent_(std::move(respondent))
    , op_id_(std::move(op_id))
    , state_name_(std::move(state_name))
    , worker_(std::move(worker))
  {}
  virtual ~ExecutionWorkload() = default;

  ExecutionWorkload(ExecutionWorkload const &other) = default;
  ExecutionWorkload &operator=(ExecutionWorkload const &other) = default;

  bool operator==(ExecutionWorkload const &other) = delete;
  bool operator<(ExecutionWorkload const &other)  = delete;

protected:
private:
  Respondent respondent_;
  OpIdent    op_id_;
  Name       state_name_;  // if empty, this operation does not need a state resource.
  Worker     worker_;
};

}  // namespace dmlf
}  // namespace fetch
