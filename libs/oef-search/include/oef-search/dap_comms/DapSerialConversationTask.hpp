#pragma once

#include <memory>
#include <queue>
#include <utility>

#include "oef-base/threading/TaskChainSerial.hpp"

#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-search/dap_comms/DapConversationTask.hpp"
#include "oef-search/dap_comms/DapParallelConversationTask.hpp"

template <typename IN_PROTO, typename OUT_PROTO, typename MIDDLE_PROTO>
class DapSerialConversationTask
  : virtual public TaskChainSerial<IN_PROTO, OUT_PROTO, DapInputDataType<MIDDLE_PROTO>,
                                   DapConversationTask<IN_PROTO, OUT_PROTO>>
{
public:
  using Parent   = TaskChainSerial<IN_PROTO, OUT_PROTO, DapInputDataType<MIDDLE_PROTO>,
                                 DapConversationTask<IN_PROTO, OUT_PROTO>>;
  using TaskType = DapConversationTask<IN_PROTO, OUT_PROTO>;

  static constexpr char const *LOGGING_NAME = "DapSerialConversationTask";

  DapSerialConversationTask(uint32_t msg_id, std::shared_ptr<OutboundConversations> outbounds,
                            std::string protocol = "dap")
    : Parent::Parent()
    , Parent()
    , msg_id_{msg_id}
    , outbounds(std::move(outbounds))
    , protocol_{std::move(protocol)}

  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task created.");
  }

  virtual ~DapSerialConversationTask()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone.");
  }

  DapSerialConversationTask(const DapSerialConversationTask &other) = delete;
  DapSerialConversationTask &operator=(const DapSerialConversationTask &other) = delete;

  bool operator==(const DapSerialConversationTask &other) = delete;
  bool operator<(const DapSerialConversationTask &other)  = delete;

  virtual std::shared_ptr<TaskType> CreateTask(const DapInputDataType<MIDDLE_PROTO> &data,
                                               std::shared_ptr<IN_PROTO>             input) override
  {
    return std::make_shared<DapConversationTask<IN_PROTO, OUT_PROTO>>(
        data.dap_name, data.path, msg_id_, input, outbounds, protocol_);
  }

protected:
  uint32_t                               msg_id_;
  std::shared_ptr<OutboundConversations> outbounds;
  std::string                            protocol_;
};
