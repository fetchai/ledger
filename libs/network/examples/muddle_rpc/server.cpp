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

#include "muddle_rpc.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

using fetch::network::NetworkManager;
using fetch::muddle::Muddle;
using fetch::muddle::NetworkId;
using fetch::muddle::rpc::Server;

int main()
{
  NetworkManager nm{"NetMgr", 1};
  nm.Start();

  Muddle muddle{NetworkId{"TEST"}, CreateKey(SERVER_PRIVATE_KEY), nm};
  muddle.Start({8000});

  Sample         sample;
  SampleProtocol sample_protocol(sample);

  Server server{muddle.AsEndpoint(), 1, 1};
  server.Add(1, &sample_protocol);

  for (;;)
  {
    sleep_for(milliseconds{500});
  }

  return 0;
}
