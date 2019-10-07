#pragma once

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
    this->SetIdentifier(ident);
    this->SetUri(uri);
    this->SetProto(initiator);
  }

  virtual ~OutboundTypedConversation() = default;

  std::vector<std::shared_ptr<PROTOCLASS>> responses;
  int                                      status_code;
  std::string                              error_message;

  virtual void HandleMessage(ConstCharArrayBuffer buffer) override
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

  virtual void HandleError(int status_code, const std::string &message) override
  {
    this->status_code = status_code;
    error_message     = message;
    wake();
  }

  virtual std::size_t GetAvailableReplyCount() const override
  {
    return responses.size();
  }
  virtual std::shared_ptr<google::protobuf::Message> GetReply(std::size_t replynumber) override
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

  virtual int GetErrorCode() const override
  {
    return status_code;
  }

  virtual const std::string &GetErrorMessage() const override
  {
    return error_message;
  }
};