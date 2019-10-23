#pragma once

namespace google {
namespace protobuf {
class Message;
};
};  // namespace google

#include <map>
#include <memory>
#include <string>

class OutboundConversation;
class IOutboundConversationCreator;
class Uri;

class OutboundConversations
{
public:
  static constexpr char const *LOGGING_NAME = "OutboundConversations";

  OutboundConversations();
  virtual ~OutboundConversations();

  // This is used to configure the system.
  void AddConversationCreator(const Uri &                                   target,
                              std::shared_ptr<IOutboundConversationCreator> creator);
  void DeleteConversationCreator(const Uri &target);

  std::shared_ptr<OutboundConversation> startConversation(
      const Uri &target, std::shared_ptr<google::protobuf::Message> initiator);

protected:
  std::map<std::string, std::shared_ptr<IOutboundConversationCreator>> creators;

private:
  OutboundConversations(const OutboundConversations &other) = delete;  // { copy(other); }
  OutboundConversations &operator                           =(const OutboundConversations &other) =
      delete;  // { copy(other); return *this; }
  bool operator==(const OutboundConversations &other) =
      delete;  // const { return compare(other)==0; }
  bool operator<(const OutboundConversations &other) =
      delete;  // const { return compare(other)==-1; }
};
