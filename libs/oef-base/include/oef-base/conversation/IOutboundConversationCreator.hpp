#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "oef-base/utils/Uri.hpp"
#include "oef-base/comms/ConstCharArrayBuffer.hpp"
#include "oef-base/conversation/OutboundConversation.hpp"

namespace google
{
  namespace protobuf
  {
    class Message;
  };
};

class IOutboundConversationCreator
{
public:
  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;

  static constexpr char const *LOGGING_NAME = "IOutboundConversationCreator";

  IOutboundConversationCreator()
  : ident2conversation_{}
  {
  }
  virtual ~IOutboundConversationCreator() = default;

  virtual std::shared_ptr<OutboundConversation> start(const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator) = 0;

  virtual void HandleMessage(unsigned long id, const Uri& uri, ConstCharArrayBuffer buffer) const
  {
    std::shared_ptr<OutboundConversation> conv = nullptr;

    {
      Lock lock(mutex_);

      auto iter = ident2conversation_.find(id);
      if (iter != ident2conversation_.end())
      {
        conv = iter->second;
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME, "complete message not handled (uri=", uri.ToString(), ")");
      }
    }

    if (conv)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "wakeup (uri=", uri.ToString(), ")!!");
      conv->HandleMessage(std::move(buffer));
    }
  }

  virtual void HandleError(unsigned long id, const Uri& uri, int status_code, const std::string &message) const
  {
    std::shared_ptr<OutboundConversation> conv = nullptr;
    {
      Lock lock(mutex_);

      auto iter = ident2conversation_.find(id);

      if (iter != ident2conversation_.end())
      {
        conv = iter->second;
      }
      else
      {
        FETCH_LOG_INFO(LOGGING_NAME, "complete message not handled (uri=", uri.ToString(), ")");
      }
    }

    if (conv)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "wakeup (uri=", uri.ToString(), ")!!");
      conv->HandleError(status_code, message);
    }
  }

protected:
  std::unordered_map<unsigned long, std::shared_ptr<OutboundConversation>> ident2conversation_;
  mutable Mutex mutex_;

private:
  IOutboundConversationCreator(const IOutboundConversationCreator &other) = delete;
  IOutboundConversationCreator &operator=(const IOutboundConversationCreator &other) = delete;
  bool operator==(const IOutboundConversationCreator &other) = delete;
  bool operator<(const IOutboundConversationCreator &other) = delete;
};
