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

#include <memory>
#include <mutex>
#include <queue>
#include <utility>

#include "core/mutex.hpp"
#include "logging/logging.hpp"
#include "oef-base/threading/TaskChainParallel.hpp"

#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-search/dap_comms/DapConversationTask.hpp"

template <typename IN_PROTO>
struct DapInputDataType
{
  std::string               dap_name;
  std::string               path;
  std::shared_ptr<IN_PROTO> proto = nullptr;
};

template <typename IN_PROTO, typename OUT_PROTO>
class DapParallelConversationTask
  : virtual public fetch::oef::base::TaskChainParallel<
        IN_PROTO, OUT_PROTO, DapInputDataType<IN_PROTO>, DapConversationTask<IN_PROTO, OUT_PROTO>>
{
public:
  using TaskType = DapConversationTask<IN_PROTO, OUT_PROTO>;
  using Parent =
      fetch::oef::base::TaskChainParallel<IN_PROTO, OUT_PROTO, DapInputDataType<IN_PROTO>,
                                          DapConversationTask<IN_PROTO, OUT_PROTO>>;
  static constexpr char const *LOGGING_NAME = "DapParallelConversationTask";

  DapParallelConversationTask(uint32_t msg_id, std::shared_ptr<OutboundConversations> outbounds,
                              std::string protocol = "dap")
    : Parent::Parent()
    , Parent()
    , msg_id_{msg_id - 1}
    , outbounds(std::move(outbounds))
    , protocol_{std::move(protocol)}

  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task created, id=", this->GetTaskId());
  }

  virtual ~DapParallelConversationTask()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone, id=", this->GetTaskId());
  }

  DapParallelConversationTask(const DapParallelConversationTask &other) = delete;
  DapParallelConversationTask &operator=(const DapParallelConversationTask &other) = delete;

  bool operator==(const DapParallelConversationTask &other) = delete;
  bool operator<(const DapParallelConversationTask &other)  = delete;

  std::shared_ptr<TaskType> CreateTask(const DapInputDataType<IN_PROTO> &data,
                                       std::shared_ptr<IN_PROTO>         input) override
  {
    return std::make_shared<DapConversationTask<IN_PROTO, OUT_PROTO>>(
        data.dap_name, data.path, ++msg_id_, input, outbounds, protocol_);
  }

  std::shared_ptr<IN_PROTO> GetInputProto(const DapInputDataType<IN_PROTO> &data) override
  {
    return data.proto;
  }

  void Add(DapInputDataType<IN_PROTO> data) override
  {
    idx_to_dap_[this->tasks_.size()] = data.dap_name;
    Parent ::Add(std::move(data));
  }

  const std::string &GetDapName(std::size_t idx) const
  {
    auto it = idx_to_dap_.find(idx);
    if (it != idx_to_dap_.end())
    {
      return it->second;
    }
    return empty_string;
  }

protected:
  uint32_t                                     msg_id_;
  std::shared_ptr<OutboundConversations>       outbounds;
  std::unordered_map<std::size_t, std::string> idx_to_dap_{};
  std::string                                  empty_string{""};
  std::string                                  protocol_;
};
