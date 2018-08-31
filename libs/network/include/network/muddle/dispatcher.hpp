#ifndef FETCH_DISPATCHER_HPP
#define FETCH_DISPATCHER_HPP

#include "core/mutex.hpp"
#include "network/muddle/packet.hpp"
#include "network/service/promise.hpp"

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <atomic>
#include <chrono>

namespace fetch {
namespace muddle {

class Dispatcher
{
public:
  using Promise = service::Promise;
  using PacketPtr = std::shared_ptr<Packet>;
  using Clock = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using Handle = uint64_t;

  static constexpr char const *LOGGING_NAME = "MuddleDispatch";

  // Construction / Destruction
  Dispatcher() = default;
  Dispatcher(Dispatcher const &) = delete;
  Dispatcher(Dispatcher &&) = delete;
  ~Dispatcher() = default;

  // Operators
  Dispatcher &operator=(Dispatcher const &) = delete;
  Dispatcher &operator=(Dispatcher &&) = delete;

  uint16_t GetNextCounter();

  Promise RegisterExchange(uint16_t service, uint16_t channel, uint16_t counter);
  bool Dispatch(PacketPtr packet);

  void NotifyMessage(Handle handle, uint16_t service, uint16_t channel, uint16_t counter);
  void NotifyConnectionFailure(Handle handle);

  void Cleanup(Timepoint const &now = Clock::now());

private:

  using Counter = std::atomic<uint16_t>;
  using Mutex = mutex::Mutex;

  struct PromiseEntry
  {
    Promise promise = service::MakePromise();
    Timepoint timestamp = Clock::now();
  };

  using PromiseMap = std::unordered_map<uint64_t, PromiseEntry>;
  using PromiseSet = std::unordered_set<uint64_t>;
  using HandleMap = std::unordered_map<Handle, PromiseSet>;

  std::atomic<uint16_t> counter_{1};

  Mutex promises_lock_{__LINE__, __FILE__};
  PromiseMap promises_;

  Mutex handles_lock_{__LINE__, __FILE__};
  HandleMap handles_;
};

inline uint16_t Dispatcher::GetNextCounter()
{
  return counter_++;
}


} // namespace muddle
} // namespace fetch

#endif //FETCH_DISPATCHER_HPP
