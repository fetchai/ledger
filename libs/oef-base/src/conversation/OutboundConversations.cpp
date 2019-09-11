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

#include "oef-base/conversation/OutboundConversations.hpp"

#include "oef-base/conversation/IOutboundConversationCreator.hpp"
#include "oef-base/utils/Uri.hpp"

OutboundConversations::OutboundConversations()
{}

OutboundConversations::~OutboundConversations()
{}

void OutboundConversations::delConversationCreator(const std::string &target)
{
  creators.erase(target);
}

void OutboundConversations::addConversationCreator(
    const std::string &target, std::shared_ptr<IOutboundConversationCreator> creator)
{
  creators[target] = creator;
}

std::shared_ptr<OutboundConversation> OutboundConversations::startConversation(
    const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator)
{
  auto iter = creators.find(target_path.host);
  if (iter != creators.end())
  {
    return iter->second->start(target_path, initiator);
  }
  throw std::invalid_argument(target_path.host);
}
