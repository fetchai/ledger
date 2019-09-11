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

#include "oef-base/conversation/OutboundConversation.hpp"

#include <google/protobuf/message.h>
#include <istream>
#include <memory>

class OutboundConversationWorkerTask;

template <class PROTOCLASS>
class OutboundTypedConversation : public OutboundConversation
{
public:
  friend class OutboundConversationWorkerTask;

  OutboundTypedConversation(std::size_t ident, Uri uri,
                            std::shared_ptr<google::protobuf::Message> initiator)
    : OutboundConversation()
  {
    this->setIdent(ident);
    this->setUri(uri);
    this->setProto(initiator);
  }

  virtual ~OutboundTypedConversation() = default;

  std::vector<std::shared_ptr<PROTOCLASS>> responses;
  int                                      status_code;
  std::string                              error_message;

  virtual void handleMessage(ConstCharArrayBuffer buffer) override
  {
    status_code    = 0;
    auto         r = std::make_shared<PROTOCLASS>();
    std::istream is(&buffer);
    if (!r->ParseFromIstream(&is))
    {
      status_code   = 91;
      error_message = "";
      wake();
      return;
    }
    responses.push_back(r);
    wake();
  }

  virtual void handleError(int status_code, const std::string &message) override
  {
    this->status_code = status_code;
    error_message     = message;
  }

  virtual std::size_t getAvailableReplyCount() const override
  {
    return responses.size();
  }
  virtual std::shared_ptr<google::protobuf::Message> getReply(std::size_t replynumber) override
  {
    if (responses.size() == 0)
    {
      return nullptr;
    }
    return responses[replynumber];
  }

  virtual bool success() const override
  {
    return status_code == 0;
  }

  virtual int getErrorCode() const override
  {
    return status_code;
  }

  virtual const std::string &getErrorMessage() const override
  {
    return error_message;
  }
};
