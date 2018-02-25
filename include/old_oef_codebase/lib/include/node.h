// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
// This file is a Node in the OEF. It currently has a agent directory and (agent) service directory
// Its responsibilities are to:
//   - Receive connections from agents and store them in its agent directory
//   - Remove agents from the agent directory when they disconnect
//   - Allow agents to register their service in the service directory
//   - Link agents to services
//   - Receive and respond to Queries
//     - Match a Query to a number of services
//     - Get a list of agent(s) from the agent directory who match those service(s)
//     - Return the agent(s)
//   - Forward messages from agent to agent

#pragma once

#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include "asio.hpp"

// Fetch libraries
#include "serviceDirectory.h"
#include "dataModelDirectory.h"
#include "messages.h"

using asio::ip::tcp;

enum class Ports
{
  ServiceDiscovery = 2222, Agents = 3333, Nodes = 4444,
};

class AgentSession;

// This class manages the AgentSessions which correspong to connections to an AEA (each AgentSession has its own socket)
class AgentDirectory
{

 public:
  AgentDirectory()                                 = default;
  AgentDirectory(const AgentDirectory &)           = delete;
  AgentDirectory operator=(const AgentDirectory &) = delete;

  // Get an AgentSession given an AEA string ID
  std::shared_ptr<AgentSession> getSession(const std::string &id) const
  {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _sessions.find(id);
    if(iter != _sessions.end())
    {
      return iter->second;
    }
    return std::shared_ptr<AgentSession>(nullptr);
  }

  // Does an agent exist/do we have a connection to them
  bool exist(const std::string &id) const
  {
    return _sessions.find(id) != _sessions.end();
  }

  bool add(const std::string &id, std::shared_ptr<AgentSession> session)
  {
    std::lock_guard<std::mutex> lock(_lock);
    if(exist(id))
    {
      return false;
    }
    _sessions[id] = session;
    return true;
  }

  bool remove(const std::string &id)
  {
    // If we are purging an AEA from the AgentDirectory, we want to kill it.
    std::lock_guard<std::mutex> lock(_lock);
    return _sessions.erase(id) == 1;
  }

  void clear()
  {
    std::lock_guard<std::mutex> lock(_lock);
    _sessions.clear();
  }

  size_t size() const
  {
    std::lock_guard<std::mutex> lock(_lock);
    return _sessions.size();
  }

  std::vector<std::string> search(const QueryModel &query) const;

 private:
  mutable std::mutex                                             _lock;
  std::unordered_map<std::string, std::shared_ptr<AgentSession>> _sessions;
};

// This is a Node.
// The AgentServer has an acceptor that will allow previously unknown AEAs to connect.
// If the connection is successful, this will spawn a new AgentSession
// AgentSessions are stored in the AgentDirectory
class AgentServer
{
 public:
  explicit AgentServer();
  AgentServer(const AgentServer &)           = delete;
  AgentServer operator=(const AgentServer &) = delete;

  ~AgentServer()
  {
    stop();
    _ad.clear(); // Is this neccessary? Won't everything destruct anyway
    if(_thread)
    {
      _thread->join();
    }
  }

  void run();
  void run_in_thread();
  size_t nbAgents() const { return _ad.size(); }
  void stop()
  {
    std::this_thread::sleep_for(std::chrono::seconds{1});
    _io_context.stop();
  }

 private:
  // Context is used during the creation of a new socket/AgentSession
  struct Context
  {
    Context(tcp::socket socket) : _socket{std::move(socket)} {}

    tcp::socket                   _socket;
    uint32_t                      _len;
    std::vector<char>             _buffer;
  };

  // Jerome: careful - the order of declaration matters.
  asio::io_context             _io_context;
  std::unique_ptr<std::thread> _thread;
  tcp::acceptor                _acceptor; // An acceptor is used for accepting new socket connections
  AgentDirectory               _ad;
  ServiceDirectory             _sd;
  DataModelDirectory           _dataModelDirectory;

  void secretHandshake(const std::string &id, const std::shared_ptr<Context> &context);
  void newSession(tcp::socket socket);
  void do_accept();
};
