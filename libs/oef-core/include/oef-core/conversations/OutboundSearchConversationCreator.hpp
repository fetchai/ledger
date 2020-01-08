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

#include <map>
#include <memory>
#include <mutex>

#include "oef-base/conversation/IOutboundConversationCreator.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/utils/Uri.hpp"

class OutboundSearchConversationWorkerTask;
template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class Core;

class OutboundSearchConversationCreator : public IOutboundConversationCreator
{
public:
  using Lock = IOutboundConversationCreator::Lock;
  using IOutboundConversationCreator::mutex_;
  using IOutboundConversationCreator::ident2conversation_;

  OutboundSearchConversationCreator(const std::string &core_key, const Uri &core_uri,
                                    const Uri &search_uri, Core &core,
                                    std::shared_ptr<OutboundConversations> outbounds);
  ~OutboundSearchConversationCreator() override;
  std::shared_ptr<OutboundConversation> start(
      const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator) override;

protected:
private:
  static constexpr char const *LOGGING_NAME = "OutboundSearchConversationCreator";
  using TXType = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;

  std::size_t ident = 1;

  std::shared_ptr<OutboundSearchConversationWorkerTask> worker;

  OutboundSearchConversationCreator(const OutboundSearchConversationCreator &other) = delete;
  OutboundSearchConversationCreator &operator=(const OutboundSearchConversationCreator &other) =
      delete;
  bool operator==(const OutboundSearchConversationCreator &other) = delete;
  bool operator<(const OutboundSearchConversationCreator &other)  = delete;
};
