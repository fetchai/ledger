#include "http/abstract_server.hpp"

namespace fetch {
namespace http {

AbstractHTTPServer::handle_type AbstractHTTPServer::global_handle_counter_ = 0;
fetch::mutex::Mutex AbstractHTTPServer::global_handle_mutex_(__LINE__, __FILE__);

} // namespace http
} // namespace fetch

