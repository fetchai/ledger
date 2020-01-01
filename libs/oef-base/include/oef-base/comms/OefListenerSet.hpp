#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <iostream>
#include <map>
#include <mutex>

#include "oef-base/comms/IOefListener.hpp"

template <class IOefTaskFactory, class OefEndpoint>
class OefListenerSet
{
public:
  using ListenerId = int;
  using Listener   = IOefListener<IOefTaskFactory, OefEndpoint>;
  using ListenerP  = std::shared_ptr<Listener>;
  using Store      = std::map<ListenerId, ListenerP>;
  using Mutex      = std::mutex;
  using Lock       = std::lock_guard<Mutex>;

  OefListenerSet()          = default;
  virtual ~OefListenerSet() = default;

  bool has(const ListenerId &id) const
  {
    Lock lock(mutex);
    return store.find(id) != store.end();
  }

  bool add(const ListenerId &id, ListenerP new_listener)
  {
    Lock lock(mutex);
    if (store.find(id) != store.end())
    {
      std::cout << "denied new listener " << id << std::endl;
      return false;
    }
    store[id] = new_listener;
    return true;
  }

  void clear(void)
  {
    store.clear();
  }

protected:
private:
  mutable Mutex mutex;
  Store         store;

  OefListenerSet(const OefListenerSet &other) = delete;
  OefListenerSet &operator=(const OefListenerSet &other)  = delete;
  bool            operator==(const OefListenerSet &other) = delete;
  bool            operator<(const OefListenerSet &other)  = delete;
};
