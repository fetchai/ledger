#include "network/muddle/dispatcher.hpp"

namespace fetch {
namespace muddle {
namespace {

const std::chrono::seconds PROMISE_TIMEOUT{30};

uint64_t Combine(uint16_t service, uint16_t channel, uint16_t counter)
{
  uint64_t id = 0;

  id |= static_cast<uint64_t>(service) << 32u;
  id |= static_cast<uint64_t>(channel) << 16u;
  id |= static_cast<uint64_t>(counter);

  return id;
}

} // namespace

Dispatcher::Promise Dispatcher::RegisterExchange(uint16_t service, uint16_t channel, uint16_t counter)
{
  FETCH_LOCK(promises_lock_);

  uint64_t const id = Combine(service, channel, counter);

  auto it = promises_.find(id);
  if (it != promises_.end())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Duplicate promise: ", service, ':', channel, ':', counter, " forced to remove entry");
    promises_.erase(it);
  }

  return promises_[id].promise;
}

bool Dispatcher::Dispatch(PacketPtr packet)
{
  bool success = false;

  FETCH_LOCK(promises_lock_);
  uint64_t const id = Combine(packet->GetService(), packet->GetProtocol(), packet->GetMessageNum());

  auto it = promises_.find(id);
  if (it != promises_.end())
  {
    assert(it->second.promise);
    it->second.promise->Fulfill(packet->GetPayload());
    success = true;
  }

  return success;
}

void Dispatcher::NotifyMessage(Handle handle, uint16_t service, uint16_t channel, uint16_t counter)
{
  FETCH_LOCK(handles_lock_);
  uint64_t const id = Combine(service, channel, counter);

  handles_[handle].insert(id);
}

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


} // namespace muddle
} // namespace fetch
