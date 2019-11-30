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

#include "block_storage_tool.hpp"

#include "logging/logging.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <thread>

namespace {

using namespace std::chrono_literals;

static constexpr char const *LOGGING_NAME = "BlkCtl";

}  // namespace

int main()
{
  fetch::crypto::mcl::details::MCLInitialiser();

  int exit_code = EXIT_FAILURE;

  try
  {
    BlockStorageTool tool;
    exit_code = tool.Run();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Fatal Error: ", ex.what());
  }

  return exit_code;
}