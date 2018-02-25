// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// This file implements a thread safe queue with functionality for timing out when 

#pragma once

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <utility>
#include <condition_variable>

#include "debugControl.h"

// Thread-safe queue with timeout functionality
template <typename T>
class Queue
{
 public:
  // Pop from queue, use timeout. This could happen when you send a message to an AEA and they just don't respond
  T pop()
  {
    std::unique_lock<std::mutex> mlock(_mutex);

    if(!_cond.wait_for(mlock,_millisecond*10000, [&]{return !_queue.empty();})) // 10 second timeout for now. TODO: (`HUT`) : Create a central definition/control for timeouts etc.
    {
      throw protocolException("Conversation queue for AEA/Node timed out!");
    }

    auto item = std::move(_queue.front());
    _queue.pop();
    return item;
  }

  T popBlocking()
  {
    std::unique_lock<std::mutex> mlock(_mutex);

    while (_queue.empty()) {
      _cond.wait(mlock);
    }

    auto item = std::move(_queue.front());
    _queue.pop();
    return item;
  }

  void push(const T& item)
  {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(item);
    mlock.unlock();
    _cond.notify_one();
  }

  void push(T&& item)
  {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(std::move(item));
    mlock.unlock();
    _cond.notify_one();
  }

  bool empty() const {
    return _queue.empty();
  }

  size_t size() const {
    return _queue.size();
  }

 private:
  std::chrono::milliseconds _millisecond{1};
  std::queue<T>             _queue;
  std::mutex                _mutex;
  std::condition_variable   _cond;
};
