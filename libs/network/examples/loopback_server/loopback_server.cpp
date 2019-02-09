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

#include "network/tcp/loopback_server.hpp"
#include <iostream>

int main(int /*argc*/, char * /*argv*/[])
{
  {
    std::cerr << "Starting loopback server" << std::endl;
    fetch::network::LoopbackServer echo(8080);

    char dummy;
    std::cout << "press any key to quit" << std::endl;
    std::cin >> dummy;
  }

  std::cout << "Finished" << std::endl;
  return 0;
}
