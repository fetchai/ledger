#include "oef-base/conversation/OutboundConversations.hpp"

#include "oef-base/conversation/IOutboundConversationCreator.hpp"
#include "oef-base/proto_comms/ProtoMessageSender.hpp"
#include "oef-base/utils/Uri.hpp"

OutboundConversations::OutboundConversations()
{}

OutboundConversations::~OutboundConversations()
{}

void OutboundConversations::delConversationCreator(const Uri &target)
{
  creators.erase(target.GetSocketAddress());
}

void OutboundConversations::addConversationCreator(
    const Uri &target, std::shared_ptr<IOutboundConversationCreator> creator)
{
  creators[target.GetSocketAddress()] = creator;
}

std::shared_ptr<OutboundConversation> OutboundConversations::startConversation(
    const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator)
{
  auto iter = creators.find(target_path.GetSocketAddress());
  if (iter != creators.end())
  {
    return iter->second->start(target_path, initiator);
  }
  FETCH_LOG_WARN(LOGGING_NAME, "Failed to create outbound conversation, because host is unknown: ",
                 target_path.GetSocketAddress());
  throw std::invalid_argument(target_path.GetSocketAddress());
}
