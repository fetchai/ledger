#include "network/service/promise.hpp"

namespace fetch {
namespace service {
namespace details {

PromiseImplementation::promise_counter_type PromiseImplementation::promise_counter_ = 0;
fetch::mutex::Mutex PromiseImplementation::counter_mutex_(__LINE__, __FILE__);

} // namespace details
} // namespace service
} // namespace fetch
