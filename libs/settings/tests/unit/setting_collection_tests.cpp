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

#include "settings/setting.hpp"
#include "settings/setting_collection.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::settings::Setting;
using fetch::settings::SettingCollection;

class SystemArgAdapter
{
public:

  SystemArgAdapter(std::vector<std::string> args)
      : args_{std::move(args)}
  {
    argv_.resize(args_.size(), nullptr);

    for (std::size_t idx = 0; idx < args_.size(); ++idx)
    {
      argv_[idx] = const_cast<char *>(args_[idx].c_str());
    }
  }

  int argc()
  {
    return static_cast<int>(argv_.size());
  }

  char **argv()
  {
    return argv_.data();
  }

private:
  std::vector<std::string> args_;
  std::vector<char *> argv_;
};

TEST(SettingCollectionTests, SimpleCheck)
{
  SettingCollection collection{};
  Setting<uint32_t> lanes{collection, "lanes", 0, ""};
  Setting<std::string> name{collection, "name", "default", ""};

  SystemArgAdapter arguments({"-lanes", "256", "-name", "foo-bar-baz"});

  collection.UpdateFromArgs(arguments.argc(), arguments.argv());

  EXPECT_EQ(lanes.value(), 256);
  EXPECT_EQ(name.value(), "foo-bar-baz");
}

} // namespace