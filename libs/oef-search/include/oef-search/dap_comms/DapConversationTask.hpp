#pragma once

#include <memory>
#include <utility>

#include "logging/logging.hpp"
#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/StateMachineTask.hpp"
#include "oef-base/utils/Uri.hpp"

template <typename IN_PROTO, typename OUT_PROTO>
class DapConversationTask
  : virtual public StateMachineTask<DapConversationTask<IN_PROTO, OUT_PROTO>>,
    virtual public Waitable
{
public:
  using StateResult    = typename StateMachineTask<DapConversationTask>::Result;
  using EntryPoint     = typename StateMachineTask<DapConversationTask>::EntryPoint;
  using MessageHandler = std::function<void(std::shared_ptr<OUT_PROTO>)>;
  using ErrorHandler =
      std::function<void(const std::string &, const std::string &, const std::string &)>;
  using SelfType = DapConversationTask<IN_PROTO, OUT_PROTO>;

  static constexpr char const *LOGGING_NAME = "DapConversationTask";

  DapConversationTask(std::string dap_name, std::string path, uint32_t msg_id,
                      std::shared_ptr<IN_PROTO>              initiator,
                      std::shared_ptr<OutboundConversations> outbounds,
                      std::string                            protocol = "dap")
    : StateMachineTask<SelfType>(this, nullptr)
    , initiator(std::move(initiator))
    , outbounds(std::move(outbounds))
    , dap_name_{std::move(dap_name)}
    , path_{std::move(path)}
    , msg_id_(msg_id)
    , protocol_{std::move(protocol)}
  {
    entryPoint.push_back(&SelfType::createConv);
    entryPoint.push_back(&SelfType::handleResponse);
    this->entrypoints = entryPoint.data();
    this->state       = this->entrypoints[0];
    FETCH_LOG_INFO(LOGGING_NAME, "DAP Conv task created: ", dap_name_);
    task_created =
        std::make_shared<Counter>("mt-search.dap." + dap_name_ + "." + path_ + ".created");
    task_errored =
        std::make_shared<Counter>("mt-search.dap." + dap_name_ + "." + path_ + ".errored");
    task_succeeded =
        std::make_shared<Counter>("mt-search.dap." + dap_name_ + "." + path_ + ".succeeded");

    (*task_created)++;

    errorHandler = [](const std::string &dap_name, const std::string &path,
                      const std::string &msg) {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to call ", path, " @ dap ", dap_name, ": ", msg);
    };

    if (!protocol_.empty())
    {
      protocol_ += "://";
    }
  }

  virtual ~DapConversationTask()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone.");
  }

  StateResult createConv(void)
  {
    try
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Start: ", protocol_, dap_name_, "/", path_);
      Uri uri(protocol_ + dap_name_ + "/" + path_);
      uri.path     = uri.path.substr(1);
      conversation = outbounds->startConversation(uri, initiator);

      FETCH_LOG_INFO(LOGGING_NAME, "Conversation created");

      auto                this_sp = this->template shared_from_base<DapConversationTask>();
      std::weak_ptr<Task> this_wp = this_sp;

      if (conversation->makeNotification()
              .Then([this_wp]() {
                auto sp = this_wp.lock();
                if (sp)
                {
                  sp->makeRunnable();
                }
              })
              .Waiting())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Sleeping");
        return DapConversationTask::StateResult(1, DEFER);
      }
      FETCH_LOG_INFO(LOGGING_NAME, "NOT Sleeping");
      return DapConversationTask::StateResult(1, COMPLETE);
    }
    catch (std::exception &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to create conversation with: ", dap_name_,
                     ", message: ", e.what());
      (*task_errored)++;
      if (errorHandler)
      {
        errorHandler(dap_name_, path_,
                     std::string{"Exception in creating conversation: "} + e.what());
      }
      wake();
      return DapConversationTask::StateResult(0, ERRORED);
    }
  }

  virtual StateResult handleResponse(void)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Woken ");
    FETCH_LOG_INFO(LOGGING_NAME, "Response.. ", conversation->getAvailableReplyCount());

    if (conversation->getAvailableReplyCount() == 0)
    {
      (*task_errored)++;
      if (errorHandler)
      {
        errorHandler(dap_name_, path_, "No response");
      }
      wake();
      return DapConversationTask::StateResult(0, ERRORED);
    }

    auto resp = conversation->getReply(0);
    if (!resp)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Got nullptr as reply");
      (*task_errored)++;
      if (errorHandler)
      {
        errorHandler(dap_name_, path_, "nullptr as reply");
      }
      wake();
      return DapConversationTask::StateResult(0, ERRORED);
    }
    auto response = std::static_pointer_cast<OUT_PROTO>(resp);
    if (messageHandler)
    {
      messageHandler(std::move(response));
      (*task_succeeded)++;
      wake();
    }
    else
    {
      (*task_errored)++;
      if (errorHandler)
      {
        errorHandler(dap_name_, path_, "no messageHandler");
      }
      wake();
      return DapConversationTask::StateResult(0, ERRORED);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "COMPLETE");

    return DapConversationTask::StateResult(0, COMPLETE);
  }

  virtual void SetMessageHandler(MessageHandler mH)
  {
    messageHandler = std::move(mH);
  }

  virtual void SetErrorHandler(ErrorHandler eH)
  {
    errorHandler = std::move(eH);
  }

  MessageHandler messageHandler;
  ErrorHandler   errorHandler;

protected:
  std::shared_ptr<IN_PROTO>              initiator;
  std::shared_ptr<OutboundConversations> outbounds;
  std::shared_ptr<OutboundConversation>  conversation;
  std::string                            dap_name_;
  std::string                            path_;
  uint32_t                               msg_id_;
  std::string                            protocol_;

  std::shared_ptr<Counter> task_created;
  std::shared_ptr<Counter> task_errored;
  std::shared_ptr<Counter> task_succeeded;

  std::vector<EntryPoint> entryPoint;

private:
  DapConversationTask(const DapConversationTask &other) = delete;  // { copy(other); }
  DapConversationTask &operator                         =(const DapConversationTask &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const DapConversationTask &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const DapConversationTask &other) =
      delete;  // const { return compare(other)==-1; }
};
