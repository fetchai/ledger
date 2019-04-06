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

#include "network/muddle/dispatcher.hpp"

namespace fetch {
namespace muddle {
namespace {

const std::chrono::seconds PROMISE_TIMEOUT{30};

/**
 * Combine service, channel and counter into a single incde
 *
 * @param service The service id
 * @param channel The channel id
 * @param counter The message counter id
 * @return The aggregated counter
 */
uint64_t Combine(uint16_t service, uint16_t channel, uint16_t counter)
{
  uint64_t id = 0;

  id |= static_cast<uint64_t>(service) << 32u;
  id |= static_cast<uint64_t>(channel) << 16u;
  id |= static_cast<uint64_t>(counter);

  return id;
}

}  // namespace

/**
 * Register that a exchange is scheduled to take place and create a promise to track the response
 *
 * @param service The service id
 * @param channel The channel id
 * @param counter The message id
 * @return The created promise for this exchange
 */
Dispatcher::Promise Dispatcher::RegisterExchange(uint16_t service, uint16_t channel,
                                                 uint16_t counter, Packet::Address const &address)
{
  FETCH_LOCK(promises_lock_);

  uint64_t const id = Combine(service, channel, counter);

  auto it = promises_.find(id);
  if (it != promises_.end())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Duplicate promise: ", service, ':', channel, ':', counter,
                    " forced to remove entry");
    promises_.erase(it);
  }

  PromiseEntry &entry = promises_[id];
  entry.address       = address;
  return entry.promise;
}

/**
 * Attempt to match an incoming packet against a pending promise
 *
 * @param packet The input packet
 * @return true if the match was found (and the packet was handled), otherwise false
 */
bool Dispatcher::Dispatch(PacketPtr packet)
{
  bool success = false;

  FETCH_LOCK(promises_lock_);
  uint64_t const id = Combine(packet->GetService(), packet->GetProtocol(), packet->GetMessageNum());

  auto it = promises_.find(id);
  if (it != promises_.end())
  {
    assert(it->second.promise);
    if (packet->GetSender() == it->second.address)
    {
      it->second.promise->Fulfill(packet->GetPayload());
      // finally remove the promise from the map (since it has been completed)
      promises_.erase(it);
      success = true;
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Recieved response from wrong address");
      FETCH_LOG_INFO(LOGGING_NAME, "Expected : " + ToBase64(it->second.address));
      FETCH_LOG_INFO(LOGGING_NAME, "Recieved : " + ToBase64(packet->GetSender()));
    }
  }

  return success;
}

/**
 * Notify the dispatcher that an exchange is taking place
 *
 * @param handle The handle to connection
 * @param service The service id
 * @param channel The channel id
 * @param counter The message number id
 */
void Dispatcher::NotifyMessage(Handle handle, uint16_t service, uint16_t channel, uint16_t counter)
{
  FETCH_LOCK(handles_lock_);
  uint64_t const id = Combine(service, channel, counter);

  handles_[handle].insert(id);
}

/**
 * Notify that a connection failure has occured and that existing pending promises should be
 * cancelled
 *
 * @param handle The conenction handle
 */
void Dispatcher::NotifyConnectionFailure(Handle handle)
{
  PromiseSet affected_promises{};

  // lookup all the affected promises
  {
    FETCH_LOCK(handles_lock_);
    auto it = handles_.find(handle);
    if (it != handles_.end())
    {
      std::swap(affected_promises, it->second);
      handles_.erase(it);
    }
  }

  // update all the affected promises
  {
    FETCH_LOCK(promises_lock_);

    for (auto const &id : affected_promises)
    {
      auto it = promises_.find(id);
      if (it != promises_.end())
      {
        it->second.promise->Fail();
        promises_.erase(it);
      }
    }
  }
}

/**
 * Run the cleanup routing
 *
 * @param now The reference time out (by default the current time)
 */
void Dispatcher::Cleanup(Timepoint const &now)
{
  FETCH_LOCK(promises_lock_);
  FETCH_LOCK(handles_lock_);

  PromiseSet dead_promises{};

  // Step 1. Determine which of the promises is now deemed to be dead
  auto promise_it = promises_.begin();
  while (promise_it != promises_.end())
  {
    auto const delta = now - promise_it->second.timestamp;
    if (delta > PROMISE_TIMEOUT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Discarding promise due to timeout");
      promise_it->second.promise->Fail();
      dead_promises.insert(promise_it->first);

      // erase the whole entry
      promise_it = promises_.erase(promise_it);
    }
    else
    {
      ++promise_it;
    }
  }

  // Step 2. Clean up the handles map
  for (auto const &id : dead_promises)
  {
    // evaluate all the of the handles
    auto handle_it = handles_.begin();
    while (handle_it != handles_.end())
    {
      auto &promise_set = handle_it->second;

      // ensure the affected promise is removed from the set
      promise_set.erase(id);

      // clear out the whole handle set if needed
      if (promise_set.empty())
      {
        handle_it = handles_.erase(handle_it);
      }
      else
      {
        ++handle_it;
      }
    }
  }
}

void Dispatcher::FailAllPendingPromises()
{
  FETCH_LOCK(promises_lock_);
  FETCH_LOCK(handles_lock_);
  for (auto promise_it = promises_.begin(); promise_it != promises_.end();)
  {
    promise_it->second.promise->Fail();
    promise_it = promises_.erase(promise_it);
  }
}

}  // namespace muddle
}  // namespace fetch
