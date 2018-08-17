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

#include "service.hpp"
#include <functional>
#include <iostream>

#include "core/commandline/parameter_parser.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
using namespace fetch::commandline;
/**
 * Please see aea.cpp for documentation.
 **/
int main(int argc, char **argv)
{

  ParamsParser params;
  params.Parse(argc, argv);

  if (params.arg_size() <= 2)
  {
    std::cout << "usage: ./" << argv[0] << " [port] [info]" << std::endl;
    return -1;
  }

  std::cout << "Starting service on " << params.GetArg<uint16_t>(1) << std::endl;

  FetchService serv(params.GetArg<uint16_t>(1), params.GetArg(2));
  serv.Start();

  // Here we create a feed of contious publication done through the
  // implementation. In our case, we make a clock that says tick/tock every half
  // second.
  bool ticktock = false;
  while (true)
  {
    ticktock = !ticktock;
    if (ticktock)
    {
      serv.Tick();
      std::cout << "Tick" << std::endl;
    }
    else
    {
      serv.Tock();
      std::cout << "Tock" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  serv.Stop();

  return 0;
}
