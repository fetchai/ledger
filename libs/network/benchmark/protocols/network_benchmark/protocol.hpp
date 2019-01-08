#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "commands.hpp"

namespace fetch {
namespace protocols {

template <typename T>
class NetworkBenchmarkProtocol : public fetch::service::Protocol
{
public:
  NetworkBenchmarkProtocol(std::shared_ptr<T> node)
    : Protocol()
  {
    this->Expose(NetworkBenchmark::INVITE_PUSH, node.get(), &T::InvitePush);
    this->Expose(NetworkBenchmark::PUSH, node.get(), &T::Push);
    this->Expose(NetworkBenchmark::PUSH_CONFIDENT, node.get(), &T::PushConfident);
    this->Expose(NetworkBenchmark::SEND_NEXT, node.get(), &T::SendNext);
    this->Expose(NetworkBenchmark::PING, node.get(), &T::ping);
  }
};

}  // namespace protocols
}  // namespace fetch
