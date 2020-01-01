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

#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Future.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Waitable.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-search/dap_comms/DapSerialConversationTask.hpp"

// DapExecute, IdentifierSequence
class NodeExecutorTask : virtual public fetch::oef::base::Task,
                         virtual public fetch::oef::base::Waitable
{
public:
  static constexpr char const *LOGGING_NAME = "NodeExecutorTask";

  using MessageHandler = std::function<void(std::shared_ptr<IdentifierSequence>)>;
  using ErrorHandler =
      std::function<void(const std::string &, const std::string &, const std::string &)>;

  NodeExecutorTask() = default;

  ~NodeExecutorTask() override                    = default;
  NodeExecutorTask(const NodeExecutorTask &other) = delete;
  NodeExecutorTask &operator=(const NodeExecutorTask &other) = delete;

  bool operator==(const NodeExecutorTask &other) = delete;
  bool operator<(const NodeExecutorTask &other)  = delete;

  virtual void SetMessageHandler(MessageHandler) = 0;
  virtual void SetErrorHandler(ErrorHandler)     = 0;

protected:
};
