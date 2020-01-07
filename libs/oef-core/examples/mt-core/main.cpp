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
#include "main.hpp"

#include <boost/program_options.hpp>

int main(int argc, char *argv[])
{
  // parse args

  std::string config_file, config_string;

  program_options::options_description desc{"Options"};

  desc.add_options()("config_file",
                     program_options::value<std::string>(&config_file)->default_value(""),
                     "Path to the configuration file.");

  desc.add_options()("config_string",
                     program_options::value<std::string>(&config_string)->default_value(""),
                     "Configuration JSON.");
  program_options::variables_map vm;
  try
  {
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN("MAIN", "Failed to parse command line arguments: ", ex.what());
    return 1;
  }
  // copy from VM to args.

  MtCore myCore;

  if (config_file.empty() && config_string.empty())
  {
    FETCH_LOG_WARN("MAIN", "Configuration not provided!");
    desc.print(std::cout);
    return 1;
  }

  if (!myCore.configure(config_file, config_string))
  {
    FETCH_LOG_WARN("MAIN", "Configuration failed, shutting down...");
    return 1;
  }

  myCore.run();
}
