//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/service_client.hpp"
#include "node_details.hpp"
#include "service_consts.hpp"
#include <iostream>
using namespace fetch::service;
using namespace fetch::byte_array;

int main()
{
  // Client setup
  fetch::network::NetworkManager tm(2);
  using client_type = fetch::network::TCPClient;
  fetch::network::ConnectionRegister<fetch::NodeDetails> creg;

  tm.Start();

  // With authentication
  {

    std::shared_ptr<fetch::service::ServiceClient> client =
        creg.CreateServiceClient<client_type>(tm, "localhost", uint16_t(8080));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    client->Call(AUTH, HELLO).Wait();

    std::cout << client->Call(TEST, GREET, "Fetch").As<std::string>() << std::endl;
  }

  // Without
  std::shared_ptr<fetch::service::ServiceClient> client =
      creg.CreateServiceClient<client_type>(tm, "localhost", uint16_t(8080));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << client->Call(TEST, GREET, "Fetch").As<std::string>() << std::endl;
  auto px = client->Call(TEST, ADD, int(2), int(3));

  tm.Stop();

  return 0;
}
