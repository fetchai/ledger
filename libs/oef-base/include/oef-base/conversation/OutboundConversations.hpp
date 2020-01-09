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

#include <map>
#include <memory>
#include <string>

class OutboundConversation;
class IOutboundConversationCreator;
class Uri;

class OutboundConversations
{
public:
  static constexpr char const *LOGGING_NAME = "OutboundConversations";

  OutboundConversations()  = default;
  ~OutboundConversations() = default;

  // This is used to configure the system.
  void AddConversationCreator(const Uri &                                   target,
                              std::shared_ptr<IOutboundConversationCreator> creator);
  void DeleteConversationCreator(const Uri &target);

  std::shared_ptr<OutboundConversation> startConversation(
      Uri const &target_path, std::shared_ptr<google::protobuf::Message> const &initiator);

protected:
  std::map<std::string, std::shared_ptr<IOutboundConversationCreator>> creators;

private:
  OutboundConversations(const OutboundConversations &other) = delete;  // { copy(other); }
  OutboundConversations &operator                           =(const OutboundConversations &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const OutboundConversations &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const OutboundConversations &other) =
      delete;  // const { return compare(other)==-1; }
};
