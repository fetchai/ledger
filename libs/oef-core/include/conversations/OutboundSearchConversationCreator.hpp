#pragma once

#include <memory>
#include <map>

#include "base/src/cpp/conversation/IOutboundConversationCreator.hpp"
#include "base/src/cpp/utils/Uri.hpp"
#include "base/src/cpp/conversation/OutboundConversations.hpp"

class OutboundSearchConversationWorkerTask;
template <typename TXType, typename Reader, typename Sender>
class ProtoMessageEndpoint;
class Core;

class OutboundSearchConversationCreator : public IOutboundConversationCreator
{
public:
  OutboundSearchConversationCreator(const std::string &core_key, const Uri &core_uri,
      const Uri &search_uri, Core &core, std::shared_ptr<OutboundConversations> outbounds);
  virtual ~OutboundSearchConversationCreator();
  virtual std::shared_ptr<OutboundConversation> start(const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator);
protected:
private:
  static constexpr char const *LOGGING_NAME = "OutboundSearchConversationCreator";
  using TXType = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;

  std::size_t ident = 1;

  std::shared_ptr<OutboundSearchConversationWorkerTask> worker;

  OutboundSearchConversationCreator(const OutboundSearchConversationCreator &other) = delete;
  OutboundSearchConversationCreator &operator=(const OutboundSearchConversationCreator &other) = delete;
  bool operator==(const OutboundSearchConversationCreator &other) = delete;
  bool operator<(const OutboundSearchConversationCreator &other) = delete;
};
