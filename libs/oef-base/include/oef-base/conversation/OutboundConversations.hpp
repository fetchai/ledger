#pragma once

namespace google
{
  namespace protobuf
  {
    class Message;
  };
};

#include <string>
#include <memory>
#include <map>

class OutboundConversation;
class IOutboundConversationCreator;
class Uri;

class OutboundConversations
{
public:
  OutboundConversations();
  virtual ~OutboundConversations();

  // This is used to configure the system.
  void addConversationCreator(const std::string &target, std::shared_ptr<IOutboundConversationCreator> creator);
  void delConversationCreator(const std::string &target);

  std::shared_ptr<OutboundConversation> startConversation(const Uri &target, std::shared_ptr<google::protobuf::Message> initiator);
protected:
  std::map<std::string, std::shared_ptr<IOutboundConversationCreator>> creators;
private:
  OutboundConversations(const OutboundConversations &other) = delete; // { copy(other); }
  OutboundConversations &operator=(const OutboundConversations &other) = delete; // { copy(other); return *this; }
  bool operator==(const OutboundConversations &other) = delete; // const { return compare(other)==0; }
  bool operator<(const OutboundConversations &other) = delete; // const { return compare(other)==-1; }
};
