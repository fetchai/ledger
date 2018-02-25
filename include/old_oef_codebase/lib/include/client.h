// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// This file is an agent. It has a proxy which will manage the connection to Node(s)
// The proxy needs to be in its own thread and will interact with the agent by allowing
// the client to pass it a function pointer to a Conversation handler


#pragma once

#include <iostream>
#include <thread>

// FETCH.ai
#include "proxy.h"
#include "messages.h"

// TODO: (`HUT`) : figure out why needed
#include "schema.h"

class Client
{

public:

  explicit Client(const std::string &id, const char *host, const std::function<void(std::unique_ptr<Conversation>)> &onNew);
  virtual ~Client();

  // These are all of the interactions we expect to be able to have with the Node (not neccessarily since we don't know the Node implementation)
  // in future there will need to be some knowledge of Node 'version' or feature set
  bool                     send(const std::string &dest, const std::string &message);
  bool                     registerAgent(const Instance &description);
  bool                     unregisterAgent(const Instance &description);
  std::vector<std::string> query(const QueryModel &query);
  std::vector<std::string> search(const QueryModel &query);
  std::vector<DataModel>   keywordLookup(const KeywordLookup &lookup);
  bool                     addDescription(const Instance &description);

  Conversation newConversation(const std::string &dest)
  {
    return Conversation(dest, _proxy);
  }

  void stop()
  {
    std::cerr << "Stopping proxy\n";
    _proxy.stop();
  }

protected:

  const std::string                          _id;
  Proxy                                      _proxy;            // Proxy is used to receive async network messages with a Node
  bool                                       secretHandshake();
  void run()
  {
    _proxy.run();
  }
};
