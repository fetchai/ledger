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

#define FETCH_DISABLE_LOGGING
#include "core/commandline/parameter_parser.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "network/service/service_client.hpp"
#include "service_consts.hpp"
#include <iostream>
using namespace fetch::commandline;
using namespace fetch::service;
using namespace fetch::byte_array;

class AEA
{
public:
  std::string SearchFor(std::string val)
  {
    std::cout << "Searching for " << val << std::endl;

    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    std::string                          ret = "";

    for (auto &s : strings_)
    {
      if (s.find(val) != std::string::npos)
      {
        ret = s;
        break;
      }
    }

    return ret;
  }

  void AddString(std::string const &s)
  {
    strings_.push_back(s);
  }

private:
  std::vector<std::string> strings_;

  fetch::mutex::Mutex mutex_{__LINE__, __FILE__};
};

class AEAProtocol : public AEA, public Protocol
{
public:
  AEAProtocol()
    : AEA()
    , Protocol()
  {
    AEA *controller = (AEA *)this;

    this->Expose(NodeToAEA::SEARCH, controller, &AEA::SearchFor);
  }

private:
};

int main(int argc, char **argv)
{
  ParamsParser params;
  params.Parse(argc, argv);

  // Client setup
  fetch::network::NetworkManager tm;
  fetch::network::TCPClient      connection(tm);
  connection.Connect("localhost", 8080);
  ServiceClient client(connection, tm);

  AEAProtocol aea_prot;

  for (std::size_t i = 0; i < params.arg_size(); ++i)
  {
    aea_prot.AddString(params.GetArg(i));
  }

  tm.Start();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  client.Add(FetchProtocols::NODE_TO_AEA, &aea_prot);

  auto p = client.Call(FetchProtocols::AEA_TO_NODE, AEAToNode::REGISTER);

  FETCH_LOG_PROMISE();
  if (p->Wait())
  {
    std::cout << "Node registered" << std::endl;

    for (;;)
    {
    }
  }

  tm.Stop();

  return 0;
}
