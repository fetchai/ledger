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

namespace google {
namespace protobuf {
class Message;
}
}  // namespace google

#include "oef-base/threading/Waitable.hpp"
#include "oef-base/utils/Uri.hpp"
#include <memory>
#include <utility>

class ConstCharArrayBuffer;

class Task;

class OutboundConversation : public fetch::oef::base::Waitable
{
protected:
  OutboundConversation() = default;

public:
  using ProtoP = std::shared_ptr<google::protobuf::Message>;

  friend class OutboundConversationWorkerTask;

  ~OutboundConversation() override = default;

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
    proto_ = std::move(proto);
  }

  virtual void SetIdentifier(unsigned long ident)
  {
    ident_ = ident;
  }

  virtual void SetId(const std::string &id)
  {
    id_ = id;
  }

  const std::string &GetId() const
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
  std::string   id_{};

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
