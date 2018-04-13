#ifndef PROTOCOLS_NETWORK_TEST_PROTOCOL_HPP
#define PROTOCOLS_NETWORK_TEST_PROTOCOL_HPP

#include"protocols/network_test/commands.hpp"
#include"chain/transaction.hpp"

namespace fetch
{
namespace protocols
{

template<typename T>
class NetworkTestProtocol : public fetch::service::Protocol {
public:

  NetworkTestProtocol(std::shared_ptr<T> node) : Protocol() {

    this->Expose(NetworkTest::SEND_TRANSACTION,
        new service::CallableClassMember<T, void(chain::Transaction trans)>
        (node.get(), &T::ReceiveTransaction));
  }
};

}
}

#endif
