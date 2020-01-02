//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "tx_storage_client.hpp"
#include "tx_storage_tool.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <thread>

namespace {

using namespace std::chrono_literals;

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromHex;

constexpr char const *LOGGING_NAME = "TxCtl";

}  // namespace

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  // parse the command line
  if (argc < 4)
  {
    std::cerr << "Usage: " << argv[0] << "<log2 lanes> <get / set> <tx hash / filename> ..."
              << std::endl;
    return EXIT_FAILURE;
  }

  auto const           log2_num_lanes = static_cast<uint32_t>(std::atoi(argv[1]));
  ConstByteArray const mode           = argv[2];

  // check the mode
  fetch::DigestSet           txs_to_get{};
  TxStorageTool::FilenameSet txs_to_set{};
  if (mode == "get")
  {
    for (int i = 3; i < argc; ++i)
    {
      txs_to_get.emplace(FromHex(argv[i]));
    }
  }
  else if (mode == "set")
  {
    for (int i = 3; i < argc; ++i)
    {
      txs_to_set.emplace(argv[i]);
    }
  }
  else
  {
    std::cerr << "Invalid mode: " << mode << std::endl;
    return EXIT_FAILURE;
  }

  try
  {
    TxStorageTool tool{log2_num_lanes};
    exit_code = tool.Run(txs_to_get, txs_to_set);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Fatal Error: ", ex.what());
  }

  return exit_code;
}
