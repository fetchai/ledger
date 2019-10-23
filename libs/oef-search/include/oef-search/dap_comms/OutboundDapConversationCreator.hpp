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

class OutboundDapConversationCreator : public IOutboundConversationCreator
{
public:
  using Lock  = IOutboundConversationCreator::Lock;
  using IOutboundConversationCreator::mutex_;
  using IOutboundConversationCreator::ident2conversation_;

  OutboundDapConversationCreator(const Uri &dap_uri, Core &core,
                                 const std::string &dap_name);
  virtual ~OutboundDapConversationCreator();
  virtual std::shared_ptr<OutboundConversation> start(
      const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator);

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
