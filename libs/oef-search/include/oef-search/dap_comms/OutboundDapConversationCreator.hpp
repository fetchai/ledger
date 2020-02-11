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

class OutboundConversationWorkerTask;
template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class Core;

class OutboundDapConversationCreator : public IOutboundConversationCreator
{
public:
  using Lock = IOutboundConversationCreator::Lock;
  using IOutboundConversationCreator::mutex_;
  using IOutboundConversationCreator::ident2conversation_;

  OutboundDapConversationCreator(const Uri &dap_uri, Core &core, const std::string &dap_name);
  ~OutboundDapConversationCreator() override;
  std::shared_ptr<OutboundConversation> start(
      const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator) override;

protected:
private:
  static constexpr char const *LOGGING_NAME = "OutboundDapConversationCreator";
  using TXType = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;

  std::size_t ident = 1;

  std::shared_ptr<OutboundConversationWorkerTask> worker;

  OutboundDapConversationCreator(const OutboundDapConversationCreator &other) = delete;
  OutboundDapConversationCreator &operator=(const OutboundDapConversationCreator &other)  = delete;
  bool                            operator==(const OutboundDapConversationCreator &other) = delete;
  bool                            operator<(const OutboundDapConversationCreator &other)  = delete;
};
