#pragma once

namespace google {
namespace protobuf {
class Message;
};
};  // namespace google

#include <memory>

#include "oef-base/threading/Waitable.hpp"
#include "oef-base/utils/Uri.hpp"

class ConstCharArrayBuffer;

class Task;

class OutboundConversation : public Waitable
{
protected:
  OutboundConversation()
  {}

public:
  using ProtoP = std::shared_ptr<google::protobuf::Message>;

  friend class OutboundConversationWorkerTask;

  virtual ~OutboundConversation()
  {}

  virtual void        HandleMessage(ConstCharArrayBuffer buffer)                       = 0;
  virtual void        HandleError(int status_code, const std::string &message)         = 0;
  virtual std::size_t GetAvailableReplyCount() const                                   = 0;
  virtual std::shared_ptr<google::protobuf::Message> GetReply(std::size_t replynumber) = 0;
  virtual bool                                       success() const                   = 0;
  virtual int                                        GetErrorCode() const              = 0;
  virtual const std::string &                        GetErrorMessage() const           = 0;

  virtual void SetUri(Uri &uri)
  {
    uri_ = uri;
  }

  virtual void SetProto(ProtoP proto)
  {
    proto_ = proto;
  }

  virtual void SetIdentifier(unsigned long ident)
  {
    ident_ = ident;
  }

  virtual void SetId(const std::string& id)
  {
    id_ = id;
  }

  const std::string& GetId() const
  {
    return id_;
  }

  virtual const unsigned long &GetIdentifier() const
  {
    return ident_;
  }

protected:
  Uri           uri_;
  ProtoP        proto_;
  unsigned long ident_;
  std::string id_{};

private:
  OutboundConversation(const OutboundConversation &other) = delete;  // { copy(other); }
  OutboundConversation &operator                          =(const OutboundConversation &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const OutboundConversation &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const OutboundConversation &other) =
      delete;  // const { return compare(other)==-1; }
};

// namespace std { template<> void swap(OutboundConversation& lhs, OutboundConversation& rhs) {
// lhs.swap(rhs); } } std::ostream& operator<<(std::ostream& os, const OutboundConversation &output)
// {}
