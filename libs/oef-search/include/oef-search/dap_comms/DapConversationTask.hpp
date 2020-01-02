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
  : virtual public fetch::oef::base::StateMachineTask<DapConversationTask<IN_PROTO, OUT_PROTO>>,
    virtual public fetch::oef::base::Waitable
{
public:
  using StateResult = typename fetch::oef::base::StateMachineTask<DapConversationTask>::Result;
  using EntryPoint  = typename fetch::oef::base::StateMachineTask<DapConversationTask>::EntryPoint;
  using MessageHandler = std::function<void(std::shared_ptr<OUT_PROTO>)>;
  using ErrorHandler =
      std::function<void(const std::string &, const std::string &, const std::string &)>;
  using SelfType = DapConversationTask<IN_PROTO, OUT_PROTO>;

  static constexpr char const *LOGGING_NAME = "DapConversationTask";

  DapConversationTask(std::string const &dap_name, std::string const &path, uint32_t msg_id,
                      std::shared_ptr<IN_PROTO>              initiator,
                      std::shared_ptr<OutboundConversations> outbounds,
                      std::string const &                    protocol = "dap")
    : fetch::oef::base::StateMachineTask<SelfType>(this, nullptr)
    , initiator(std::move(initiator))
    , outbounds(std::move(outbounds))
    , dap_name_{std::move(dap_name)}
    , path_{std::move(path)}
    , msg_id_(msg_id)
    , protocol_{std::move(protocol)}
  {
    entryPoint.push_back(&SelfType::CreateConversation);
    entryPoint.push_back(&SelfType::HandleResponse);
    this->entrypoints = entryPoint.data();
    this->state       = this->entrypoints[0];
    FETCH_LOG_INFO(LOGGING_NAME, "DAP Conv task created: ", dap_name_, ", id=", this->GetTaskId());
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

  ~DapConversationTask() override
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone, id=", this->GetTaskId());
  }

  StateResult CreateConversation()
  {
    try
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Start: ", protocol_, dap_name_, "/", path_,
                     ", id=", this->GetTaskId());
      Uri uri(protocol_ + dap_name_ + "/" + path_);
      if (!protocol_.empty())
      {
        uri.path = uri.path.substr(1);
      }
      conversation = outbounds->startConversation(uri, initiator);

      FETCH_LOG_INFO(LOGGING_NAME, "Conversation created with ", uri.ToString());

      auto this_sp = this->template shared_from_base<DapConversationTask>();
      std::weak_ptr<fetch::oef::base::Task> this_wp = this_sp;

      if (conversation->MakeNotification()
              .Then([this_wp]() {
                auto sp = this_wp.lock();
                if (sp)
                {
                  sp->MakeRunnable();
                }
              })
              .Waiting())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Sleeping (id=", this->GetTaskId(), ", uri=", uri.ToString(),
                       ")");
        return DapConversationTask::StateResult(1, fetch::oef::base::DEFER);
      }
      FETCH_LOG_INFO(LOGGING_NAME, "NOT Sleeping (", uri.ToString(), ")");
      return DapConversationTask::StateResult(1, fetch::oef::base::COMPLETE);
    }
    catch (std::exception const &e)
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
      return DapConversationTask::StateResult(0, fetch::oef::base::ERRORED);
    }
  }

  virtual StateResult HandleResponse()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Woken (", dap_name_, ")");
    FETCH_LOG_INFO(LOGGING_NAME, "Response from ", dap_name_, ": ",
                   conversation->GetAvailableReplyCount());

    if (conversation->GetAvailableReplyCount() == 0)
    {
      (*task_errored)++;
      if (errorHandler)
      {
        errorHandler(dap_name_, path_, "No response");
      }
      wake();
      return DapConversationTask::StateResult(0, fetch::oef::base::ERRORED);
    }

    auto resp = conversation->GetReply(0);
    if (!resp)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Got nullptr as reply from ", dap_name_);
      (*task_errored)++;
      if (errorHandler)
      {
        errorHandler(dap_name_, path_, "nullptr as reply");
      }
      wake();
      return DapConversationTask::StateResult(0, fetch::oef::base::ERRORED);
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
      return DapConversationTask::StateResult(0, fetch::oef::base::ERRORED);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "COMPLETE (", dap_name_, ")");

    return DapConversationTask::StateResult(0, fetch::oef::base::COMPLETE);
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
