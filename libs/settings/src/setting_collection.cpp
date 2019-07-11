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

#include "core/commandline/parameter_parser.hpp"
#include "settings/setting_base.hpp"
#include "settings/setting_collection.hpp"

#include <algorithm>
#include <cassert>

namespace fetch {
namespace settings {
namespace {

/**
 * Transforms the input command line name into a corresponding environment variable name.
 *
 * i.e. foo-bar -> CONSTELLATION_FOO_BAR
 *
 * @param name The command line argument name
 * @return The environment variable name
 */
std::string GetEnvironmentVariableName(char const *prefix, std::string const &name)
{
  std::string env_name{name};

  std::transform(env_name.begin(), env_name.end(), env_name.begin(), [](char c) {
    if (std::isalnum(c))
    {
      c = static_cast<char>(std::toupper(c));
    }
    else if (c == '-')
    {
      c = '_';
    }
    return c;
  });

  return prefix + env_name;
}

/**
 * Get the corresponding value for the environment variable
 * @param name
 * @return
 */
char const *GetEnvironmentVariable(char const *prefix, std::string const &name)
{
  // determine the environment variable name
  std::string env_name = GetEnvironmentVariableName(prefix, name);

  return std::getenv(env_name.c_str());
}

}  // namespace

using fetch::commandline::ParamsParser;

/**
 * Adds a new setting to the collection
 *
 * @param setting The collection to be added
 */
void SettingCollection::Add(SettingBase &setting)
{
  settings_.push_back(&setting);
}

/**
 * Attempt to update the settings
 *
 * @param argc
 * @param argv
 */
void SettingCollection::UpdateFromArgs(int argc, char **argv)
{
  ParamsParser parser;
  parser.Parse(argc, argv);

  for (auto *setting : settings_)
  {
    assert(setting);

    std::string cmd_value{};
    if (parser.LookupParam(setting->name(), cmd_value))
    {
      // update the setting
      std::istringstream iss{cmd_value};
      iss >> *setting;
    }
  }
}

void SettingCollection::UpdateFromEnv(char const *prefix)
{
  for (auto *setting : settings())
  {
    assert(setting);

    auto const *env_value = GetEnvironmentVariable(prefix, setting->name());
    if (env_value)
    {
      std::istringstream iss{env_value};
      iss >> *setting;
    }
  }
}

}  // namespace settings
}  // namespace fetch
