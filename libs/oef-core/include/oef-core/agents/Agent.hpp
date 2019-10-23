#pragma once

#include <memory>
#include <string>

#include "oef-base/threading/Notification.hpp"

namespace google {
namespace protobuf {
class Message;
};
};  // namespace google

class OefAgentEndpoint;

class Agent
{
public:
  Agent(const std::string &key, std::shared_ptr<OefAgentEndpoint> endpoint)
  {
    this->key      = key;
    this->endpoint = endpoint;
  }
  virtual ~Agent()
  {}

  Notification::NotificationBuilder send(std::shared_ptr<google::protobuf::Message> s);

  void run_sending();

  std::string getPublicKey()
  {
    return key;
  }

protected:
private:
  std::string                       key;
  std::shared_ptr<OefAgentEndpoint> endpoint;

  Agent(const Agent &other) = delete;
  Agent &operator=(const Agent &other)  = delete;
  bool   operator==(const Agent &other) = delete;
  bool   operator<(const Agent &other)  = delete;
};
