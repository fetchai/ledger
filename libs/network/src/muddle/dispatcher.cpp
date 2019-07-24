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

#include "core/time/to_seconds.hpp"
#include "network/muddle/dispatcher.hpp"
#include "network/muddle/network_id.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"

namespace fetch {
namespace muddle {
namespace {

using std::chrono::duration_cast;
using std::chrono::microseconds;
using telemetry::Registry;

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
  LOG_STACK_TRACE_POINT;
  uint64_t id = 0;

  id |= static_cast<uint64_t>(service) << 32u;
  id |= static_cast<uint64_t>(channel) << 16u;
  id |= static_cast<uint64_t>(counter);

  return id;
}

}  // namespace

Dispatcher::Dispatcher(NetworkId const &network_id, Packet::Address const &address)
  : exchange_success_totals_{Registry::Instance().CreateCounter(
        "ledger_muddle_exchange_success_total", "The total number of successful exchanges",
        {{"network_id", network_id.ToString()},
         {"address", static_cast<std::string>(address.ToBase64())}})}
  , exchange_failure_totals_{Registry::Instance().CreateCounter(
        "ledger_muddle_exchange_failure_total", "The total number of failed exchanges",
        {{"network_id", network_id.ToString()},
         {"address", static_cast<std::string>(address.ToBase64())}})}
  , exchange_times_{Registry::Instance().CreateHistogram(
        {0.000001, 0.00001, 0.0001, 0.001, 0.01, 0.1, 1, 10., 100.}, "ledger_muddle_exchange_times",
        "The histogram of exchange times",
        {{"network_id", network_id.ToString()},
         {"address", static_cast<std::string>(address.ToBase64())}})}
  , exchange_time_max_{Registry::Instance().CreateGauge<double>(
        "ledger_muddle_exchange_time_max", "The largest exchange time observed",
        {{"network_id", network_id.ToString()},
         {"address", static_cast<std::string>(address.ToBase64())}})}
{
}

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
  LOG_STACK_TRACE_POINT;
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
  LOG_STACK_TRACE_POINT;
  bool success = false;

  double duration_secs{0.0};
  {
    FETCH_LOCK(promises_lock_);
    uint64_t const id =
        Combine(packet->GetService(), packet->GetProtocol(), packet->GetMessageNum());

    auto it = promises_.find(id);
    if (it != promises_.end())
    {
      assert(it->second.promise);
      if (packet->GetSender() == it->second.address)
      {
        // capture the network time
        duration_secs = ToSeconds(Clock::now() - it->second.timestamp);

        // fulfill the pending promise
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
  }

  // telemetry
  if (success)
  {
    exchange_success_totals_->increment();
    exchange_times_->Add(duration_secs);
    exchange_time_max_->max(duration_secs);
  }
  else
  {
    exchange_failure_totals_->increment();
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
  LOG_STACK_TRACE_POINT;
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
  LOG_STACK_TRACE_POINT;
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
  LOG_STACK_TRACE_POINT;
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
  LOG_STACK_TRACE_POINT;
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
