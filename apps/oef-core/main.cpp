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

#include "MtCore.hpp"

int main(int argc, char *argv[])
{
  // parse args

  std::string config_file;

  if (argc != 2)
  {
    FETCH_LOG_ERROR("MTCoreApp", "Failed to run binary, because exactly 1 argument ",
                    "(path to config file) should be passed!");
    return -1;
  }

  config_file = argv[1];

  MtCore myCore;

  if (config_file.empty())
  {
    FETCH_LOG_WARN("MTCoreApp", "Configuration not provided!");
    return 1;
  }

  if (!myCore.configure(config_file, ""))
  {
    FETCH_LOG_WARN("MTCoreApp", "Configuration failed, shutting down...");
    return 1;
  }

  myCore.run();
}
