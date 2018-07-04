#pragma once

#include"./commands.hpp"

namespace fetch
{
namespace protocols
{

template<typename T>
class NetworkMineTestProtocol : public fetch::service::Protocol {
public:

  NetworkMineTestProtocol(std::shared_ptr<T> node) : Protocol() {
    this->Expose(NetworkMineTest::PUSH_NEW_HEADER,    node.get(),  &T::ReceiveNewHeader);
    this->Expose(NetworkMineTest::PROVIDE_HEADER,      node.get(),  &T::ProvideHeader);
  }
};

}
}

