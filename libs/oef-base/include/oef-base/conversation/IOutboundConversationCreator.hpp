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
#include <unordered_map>

#include "oef-base/comms/ConstCharArrayBuffer.hpp"
#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/utils/Uri.hpp"

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

class IOutboundConversationCreator
{
public:
  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;

  static constexpr char const *LOGGING_NAME = "IOutboundConversationCreator";

  IOutboundConversationCreator()          = default;
  virtual ~IOutboundConversationCreator() = default;

  virtual std::shared_ptr<OutboundConversation> start(
      const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator) = 0;

  virtual void HandleMessage(unsigned long id, const Uri &uri,
                             ConstCharArrayBuffer const &buffer) const
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
      conv->HandleMessage(buffer);
    }
  }

  virtual void HandleError(unsigned long id, const Uri &uri, int status_code,
                           const std::string &message) const
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
  mutable Mutex                                                            mutex_;

private:
  IOutboundConversationCreator(const IOutboundConversationCreator &other) = delete;
  IOutboundConversationCreator &operator=(const IOutboundConversationCreator &other)  = delete;
  bool                          operator==(const IOutboundConversationCreator &other) = delete;
  bool                          operator<(const IOutboundConversationCreator &other)  = delete;
};
