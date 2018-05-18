#ifndef PROTOCOLS_NETWORK_BENCHMARK_PROTOCOL_HPP
#define PROTOCOLS_NETWORK_BENCHMARK_PROTOCOL_HPP

#include"./commands.hpp"

namespace fetch
{
namespace protocols
{

template<typename T>
class NetworkBenchmarkProtocol : public fetch::service::Protocol {
public:

  NetworkBenchmarkProtocol(std::shared_ptr<T> node) : Protocol() {
    this->Expose(NetworkBenchmark::INVITE_PUSH, node.get(),  &T::InvitePush);
    this->Expose(NetworkBenchmark::PUSH, node.get(),  &T::Push);
    this->Expose(NetworkBenchmark::PING, node.get(),  &T::ping);
  }
};

}
}

#endif
