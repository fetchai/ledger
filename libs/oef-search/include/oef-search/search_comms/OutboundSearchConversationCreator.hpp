#pragma once

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

class OutboundSearchConversationCreator : public IOutboundConversationCreator
{
public:
  using Lock  = IOutboundConversationCreator::Lock;
  using IOutboundConversationCreator::mutex_;
  using IOutboundConversationCreator::ident2conversation_;

  OutboundSearchConversationCreator(const Uri &search_uri, Core &core);
  virtual ~OutboundSearchConversationCreator();
  virtual std::shared_ptr<OutboundConversation> start(
      const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator);

protected:
private:
  static constexpr char const *LOGGING_NAME = "OutboundSearchConversationCreator";
  using TXType = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;

  std::size_t ident = 1;

  std::shared_ptr<OutboundConversationWorkerTask> worker;

  Uri search_uri_;

  OutboundSearchConversationCreator(const OutboundSearchConversationCreator &other) = delete;
  OutboundSearchConversationCreator &operator=(const OutboundSearchConversationCreator &other) =
      delete;
  bool operator==(const OutboundSearchConversationCreator &other) = delete;
  bool operator<(const OutboundSearchConversationCreator &other)  = delete;
};
