#include "network/management/abstract_connection.hpp"

namespace fetch {
namespace network {

AbstractConnection::connection_handle_type
                    AbstractConnection::global_handle_counter_ = 0;
fetch::mutex::Mutex AbstractConnection::global_handle_mutex_(__LINE__,
                                                             __FILE__);

}  // namespace network
}  // namespace fetch
