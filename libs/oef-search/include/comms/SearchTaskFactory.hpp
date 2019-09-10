#pragma once

#include "base/src/cpp/comms/IOefTaskFactory.hpp"
#include "base/src/cpp/utils/Uri.hpp"

class OefSearchEndpoint;
class OutboundConversations;

class SearchTaskFactory : public IOefTaskFactory<OefSearchEndpoint>
{
public:

  SearchTaskFactory(std::shared_ptr<OefSearchEndpoint> the_endpoint, std::shared_ptr<OutboundConversations> outbounds)
  : IOefTaskFactory(the_endpoint, outbounds)
  {
  }

  virtual ~SearchTaskFactory()
  {
  }

  virtual void processMessage(ConstCharArrayBuffer &data)
  {
    current_uri_.diagnostic();
    data.diagnostic();
  }
  // Process the message, throw exceptions if they're bad.

  Uri current_uri_;
protected:


  virtual void endpointClosed(void)
  {

  }

private:

  SearchTaskFactory(const SearchTaskFactory &other) = delete;
  SearchTaskFactory &operator=(const SearchTaskFactory &other) = delete;
  bool operator==(const SearchTaskFactory &other) = delete;
  bool operator<(const SearchTaskFactory &other) = delete;

};
