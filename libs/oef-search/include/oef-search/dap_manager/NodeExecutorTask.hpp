#pragma once

#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Future.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Waitable.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-search/dap_comms/DapSerialConversationTask.hpp"

// DapExecute, IdentifierSequence
class NodeExecutorTask : virtual public Task, virtual public Waitable
{
public:
  static constexpr char const *LOGGING_NAME = "NodeExecutorTask";

  using MessageHandler = std::function<void(std::shared_ptr<IdentifierSequence>)>;
  using ErrorHandler =
      std::function<void(const std::string &, const std::string &, const std::string &)>;

  NodeExecutorTask()
  {}

  virtual ~NodeExecutorTask()                     = default;
  NodeExecutorTask(const NodeExecutorTask &other) = delete;
  NodeExecutorTask &operator=(const NodeExecutorTask &other) = delete;

  bool operator==(const NodeExecutorTask &other) = delete;
  bool operator<(const NodeExecutorTask &other)  = delete;

  virtual void SetMessageHandler(MessageHandler) = 0;
  virtual void SetErrorHandler(ErrorHandler)     = 0;

protected:
};
