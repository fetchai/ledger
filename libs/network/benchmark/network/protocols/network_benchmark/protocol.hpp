#pragma once

#include "./commands.hpp"

namespace fetch {
namespace protocols {

template <typename T>
class NetworkBenchmarkProtocol : public fetch::service::Protocol
{
public:
  NetworkBenchmarkProtocol(std::shared_ptr<T> node) : Protocol()
  {
    this->Expose(NetworkBenchmark::INVITE_PUSH, node.get(), &T::InvitePush);
    this->Expose(NetworkBenchmark::PUSH, node.get(), &T::Push);
    this->Expose(NetworkBenchmark::PUSH_CONFIDENT, node.get(),
                 &T::PushConfident);
    this->Expose(NetworkBenchmark::SEND_NEXT, node.get(), &T::SendNext);
    this->Expose(NetworkBenchmark::PING, node.get(), &T::ping);
  }
};

}  // namespace protocols
}  // namespace fetch

