#include "base/src/cpp/conversation/OutboundConversations.hpp"

#include "base/src/cpp/conversation/IOutboundConversationCreator.hpp"
#include "base/src/cpp/utils/Uri.hpp"

OutboundConversations::OutboundConversations()
{
}

OutboundConversations::~OutboundConversations()
{
}

void OutboundConversations::delConversationCreator(const std::string &target)
{
  creators.erase(target);
}

void OutboundConversations::addConversationCreator(const std::string &target, std::shared_ptr<IOutboundConversationCreator> creator)
{
  creators[target] = creator;
}

std::shared_ptr<OutboundConversation> OutboundConversations::startConversation(const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator)
{
  auto iter = creators.find(target_path.host);
  if (iter != creators.end())
  {
    return iter -> second -> start(target_path, initiator);
  }
  throw std::invalid_argument(target_path.host);
}
