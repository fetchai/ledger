// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// Proxy opens and manages a connection to the Node. Messages received by are passed to the object holding the proxy by using
// a function pointer that takes a Conversation (onNew)

#pragma once

#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <utility>
#include <memory>
#include <string>
#include "asio.hpp"

// Fetch.ai
#include "queue.h"
#include "messages.h"
#include "serialize.h"
#include "uuid.h"
#include "common.h"

using asio::ip::tcp;

class Conversation;

class Proxy
{
 public:
  explicit Proxy(const std::string &host, const std::string &port, const std::function<void(std::unique_ptr<Conversation>)> onNew) :
    _socket{_io_context},
    _onNew{std::move(onNew)}
  {
    tcp::resolver resolver(_io_context);
    asio::connect(_socket, resolver.resolve(host, port));
  }

  ~Proxy()
  {
    _socket.shutdown(asio::socket_base::shutdown_both);
    _socket.close();
    _io_context.stop(); // ?????
    _proxyThread->join();
  }

  void run()
  {
    if (!_proxyThread)
    {
      _proxyThread = std::unique_ptr<std::thread>(new std::thread([this]() { read(); _io_context.run();}));
    }
  }

  void stop()
  {
    _io_context.stop(); // ?????
  }

  void push(const std::string &msg)
  {
    write(msg);
  }

  // Get the messages we have received in association with a certain AEA (uuid)
  Queue<std::string> &getQueue(const std::string &uuid)
  {
    std::unique_lock<std::mutex> mlock(_mutex);
    return _inMsgBox[uuid];
  }

  std::string pop(const std::string &uuid)
  {
    std::unique_lock<std::mutex> mlock(_mutex);
    auto iter = _inMsgBox.find(uuid);
    if (iter == _inMsgBox.end())
    {
      auto &q = _inMsgBox[uuid];
      mlock.unlock();
      return q.pop();
    }
    mlock.unlock();
    return iter->second.pop();
  }

 private:
  asio::io_context                                    _io_context;
  tcp::socket                                         _socket;
  std::unordered_map<std::string, Queue<std::string>> _inMsgBox;
  std::mutex                                          _mutex;
  std::unique_ptr<std::thread>                        _proxyThread;
  std::function<void(std::unique_ptr<Conversation>)>  _onNew;

  void read();
  void write(const std::string &msg)
  {
    asyncWrite(_socket, msg);
  }
};

// The Conversation class represents an open channel between the AEA and another AEA (the Node does the job of forwarding)
// Since receiving messages is an asynchronous phenomonon, the proxy manages a Queue of the messages received
class Conversation
{
 public:
  // Constructor in the case that our conversation is part of a queue (we have an uuid here as it's from someone) TODO: (`HUT`) : confirm this
  Conversation(const std::string &uuid, const std::string &dest, Queue<std::string> &queue, Proxy &proxy) :
    _uuid{Uuid{uuid}},
    _dest{dest},
    _queue{queue},
    _proxy{proxy} {}

  // Constructor in the case we are creating a Conversation with some other AEA
  // In this case we need to randomly create a uuid
  Conversation(const std::string &dest, Proxy &proxy) :
    _uuid{Uuid::uuid4()},
    _dest{dest},
    _queue{proxy.getQueue(_uuid.to_string())},
    _proxy{proxy} {}

  Conversation(const Conversation &other)       = default;
  Conversation &operator=(Conversation &&other) = default;

  // We have to allow for the possibility of not getting a response
  std::string popString(bool isBlocking = false)
  {
    if (isBlocking)
    {
      return _queue.popBlocking();
    } else {
      return _queue.pop();
    }
  }

  template <typename T>
  bool sendMsg(const T &t)
  {
    return send(toJsonString<T>(t));
  }

  AgentMessage pop(bool isBlocking = false)
  {
    std::string answer = popString();
    std::cerr << "Answer received\n";
    AgentMessage am = fromJsonString<AgentMessage>(answer);
    return am;
  }

  template <typename T>
  bool popMsg(T &t, bool isBlocking = false)
  {
    try {
      AgentMessage am = pop(isBlocking);
      std::cerr << ">>>From " << am.origin() << " content " << am.content() << std::endl;
      t = fromJsonString<T>(am.content());

      std::cerr << "returning: " << toJsonString(t) << std::endl;
      return true;
    } catch(...) {
      return false;
    }
  }

  size_t      nbMsgs() const { return _queue.size(); }
  std::string dest() const { return _dest; }
  bool        send(const std::string &message);

 private:
  Uuid                     _uuid;
  std::string              _dest;
  Queue<std::string>       &_queue;
  Proxy                    &_proxy;
};
