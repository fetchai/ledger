#pragma once

#include <memory>
#include <map>

#include "base/src/cpp/conversation/IOutboundConversationCreator.hpp"
#include "base/src/cpp/utils/Uri.hpp"
#include "base/src/cpp/conversation/OutboundConversations.hpp"

class OutboundConversationWorkerTask;
template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class Core;

class OutboundDapConversationCreator : public IOutboundConversationCreator
{
public:
  OutboundDapConversationCreator(size_t thread_group_id, const Uri &dap_uri, Core &core, std::shared_ptr<OutboundConversations> outbounds);
  virtual ~OutboundDapConversationCreator();
  virtual std::shared_ptr<OutboundConversation> start(const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator);
protected:
private:
  static constexpr char const *LOGGING_NAME = "OutboundDapConversationCreator";
  using TXType = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;

  std::size_t ident = 1;

  std::shared_ptr<OutboundConversationWorkerTask> worker;

  std::map<unsigned long, std::shared_ptr<OutboundConversation>> ident2conversation;

  OutboundDapConversationCreator(const OutboundDapConversationCreator &other) = delete;
  OutboundDapConversationCreator &operator=(const OutboundDapConversationCreator &other) = delete;
  bool operator==(const OutboundDapConversationCreator &other) = delete;
  bool operator<(const OutboundDapConversationCreator &other) = delete;
};
