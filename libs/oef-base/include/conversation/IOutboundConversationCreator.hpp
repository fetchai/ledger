#pragma once

#include <memory>

class Uri;
class OutboundConversation;

namespace google
{
  namespace protobuf
  {
    class Message;
  };
};

class IOutboundConversationCreator
{
public:
  IOutboundConversationCreator()
  {
  }
  virtual ~IOutboundConversationCreator()
  {
  }

  virtual std::shared_ptr<OutboundConversation> start(const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator) = 0;
protected:
private:
  IOutboundConversationCreator(const IOutboundConversationCreator &other) = delete;
  IOutboundConversationCreator &operator=(const IOutboundConversationCreator &other) = delete;
  bool operator==(const IOutboundConversationCreator &other) = delete;
  bool operator<(const IOutboundConversationCreator &other) = delete;
};
