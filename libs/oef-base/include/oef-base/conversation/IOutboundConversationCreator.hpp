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

#include <memory>

class Uri;
class OutboundConversation;

namespace google {
namespace protobuf {
class Message;
};
};  // namespace google

class IOutboundConversationCreator
{
public:
  IOutboundConversationCreator()
  {}
  virtual ~IOutboundConversationCreator()
  {}

  virtual std::shared_ptr<OutboundConversation> start(
      const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator) = 0;

protected:
private:
  IOutboundConversationCreator(const IOutboundConversationCreator &other) = delete;
  IOutboundConversationCreator &operator=(const IOutboundConversationCreator &other)  = delete;
  bool                          operator==(const IOutboundConversationCreator &other) = delete;
  bool                          operator<(const IOutboundConversationCreator &other)  = delete;
};
