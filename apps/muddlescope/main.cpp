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

#include "scope_client.hpp"

#include "core/commandline/params.hpp"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

namespace {

using fetch::byte_array::ConstByteArray;
using std::chrono::milliseconds;

using ClientPtr = std::shared_ptr<ScopeClient>;

using DispatchFunction =
    std::function<void(ClientPtr const &, ConstByteArray const &host, uint16_t port)>;
using DispatchMap = std::unordered_map<std::string, DispatchFunction>;

DispatchMap dispatch_map = {{"ping", [](ClientPtr const &client, ConstByteArray const &host,
                                        uint16_t port) { client->Ping(host, port); }}};

}  // namespace

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " <host> <port> <command>" << std::endl;
    return exit_code;
  }

  // extract the command line arguments
  ConstByteArray const host{argv[1]};
  uint16_t const       port = static_cast<uint16_t>(std::atoi(argv[2]));
  std::string const    command{argv[3]};

  // create the scope client
  auto client = std::make_shared<ScopeClient>();

  auto it = dispatch_map.find(command);
  if (it == dispatch_map.end())
  {
    std::cerr << "Unable to find command: " << command << std::endl;
    return exit_code;
  }

  // dipatch the executor
  try
  {
    // execute the dispatcher
    it->second(client, host, port);

    // update the exit code
    exit_code = EXIT_SUCCESS;
  }
  catch (std::exception const &ex)
  {
    std::cerr << "Error: " << ex.what() << std::endl;
  }

  return exit_code;
}
