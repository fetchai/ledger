#pragma once
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

#include "settings/detail/environment_wrapper.hpp"

#include <vector>

namespace fetch {
namespace settings {

class SettingBase;

/**
 * A very simple collection of pointers to settings object. The use must make sure that the
 * lifetime of the settings is longer than this container
 */
class SettingCollection
{
public:
  using Settings = std::vector<SettingBase *>;

  Settings const &settings() const;

  void Add(SettingBase &setting);
  void UpdateFromArgs(int argc, char **argv);
  void UpdateFromEnv(char const *                        prefix,
                     detail::EnvironmentInterface const &env = detail::Environment{});

private:
  Settings settings_;
};

/**
 * Get the current array of settings
 *
 * @return The array of settings
 */
inline SettingCollection::Settings const &SettingCollection::settings() const
{
  return settings_;
}

}  // namespace settings
}  // namespace fetch
