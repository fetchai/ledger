#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "core/mutex.hpp"
#include "network/muddle/packet.hpp"
#include "network/service/promise.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace muddle {

class Dispatcher
{
public:
  using Promise   = service::Promise;
  using PacketPtr = std::shared_ptr<Packet>;
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using Handle    = uint64_t;
  using Address   = Packet::Address;

  static constexpr char const *LOGGING_NAME = "MuddleDispatch";

  // Construction / Destruction
  Dispatcher()                   = default;
  Dispatcher(Dispatcher const &) = delete;
  Dispatcher(Dispatcher &&)      = delete;
  ~Dispatcher()                  = default;

  // Operators
  Dispatcher &operator=(Dispatcher const &) = delete;
  Dispatcher &operator=(Dispatcher &&) = delete;

  uint16_t GetNextCounter();

  Promise RegisterExchange(uint16_t service, uint16_t channel, uint16_t counter,
                           Packet::Address const &address);
  bool    Dispatch(PacketPtr packet);

  void NotifyMessage(Handle handle, uint16_t service, uint16_t channel, uint16_t counter);
  void NotifyConnectionFailure(Handle handle);

  void Cleanup(Timepoint const &now = Clock::now());

  void FailAllPendingPromises();

private:
  using Counter = std::atomic<uint16_t>;
  using Mutex   = mutex::Mutex;

  struct PromiseEntry
  {
    Promise   promise   = service::MakePromise();
    Timepoint timestamp = Clock::now();
    Address   address;
  };

  using PromiseMap = std::unordered_map<uint64_t, PromiseEntry>;
  using PromiseSet = std::unordered_set<uint64_t>;
  using HandleMap  = std::unordered_map<Handle, PromiseSet>;

  Mutex    counter_lock_{__LINE__, __FILE__};
  uint16_t counter_{1};

  Mutex      promises_lock_{__LINE__, __FILE__};
  PromiseMap promises_;

  Mutex     handles_lock_{__LINE__, __FILE__};
  HandleMap handles_;
};

inline uint16_t Dispatcher::GetNextCounter()
{
  FETCH_LOCK(counter_lock_);
  return counter_++;
}

}  // namespace muddle
}  // namespace fetch
