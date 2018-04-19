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
    this->Expose(NetworkBenchmark::SEND_TRANSACTIONS,  node.get(),  &T::ReceiveTransactions);
  }
};

}
}

#endif
