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
#include <cstdint>
#include <string>
#include <vector>

namespace {

using fetch::settings::Setting;
using fetch::settings::SettingCollection;

TEST(SettingTests, CheckUInt32)
{
  SettingCollection collection{};
  Setting<uint32_t> setting{collection, "foo", 0, "A sample setting"};

  EXPECT_EQ(setting.name(), "foo");
  EXPECT_EQ(setting.description(), "A sample setting");
  EXPECT_EQ(setting.default_value(), 0);
  EXPECT_EQ(setting.value(), 0u);

  std::istringstream iss{"401"};
  setting.FromStream(iss);

  EXPECT_EQ(setting.name(), "foo");
  EXPECT_EQ(setting.description(), "A sample setting");
  EXPECT_EQ(setting.default_value(), 0u);
  EXPECT_EQ(setting.value(), 401u);
}

TEST(SettingTests, CheckSizet)
{
  SettingCollection collection{};
  Setting<std::size_t> setting{collection, "block-interval", 250u, "A sample setting"};

  EXPECT_EQ(setting.name(), "block-interval");
  EXPECT_EQ(setting.description(), "A sample setting");
  EXPECT_EQ(setting.default_value(), 250u);
  EXPECT_EQ(setting.value(), 250u);

  std::istringstream iss{"40100"};
  setting.FromStream(iss);

  EXPECT_EQ(setting.name(), "block-interval");
  EXPECT_EQ(setting.description(), "A sample setting");
  EXPECT_EQ(setting.default_value(), 250u);
  EXPECT_EQ(setting.value(), 40100u);
}

TEST(SettingTests, CheckDouble)
{
  SettingCollection collection{};
  Setting<double> setting{collection, "threshold", 10.0, "A sample setting"};

  EXPECT_EQ(setting.name(), "threshold");
  EXPECT_EQ(setting.description(), "A sample setting");
  EXPECT_DOUBLE_EQ(setting.default_value(), 10.0);
  EXPECT_DOUBLE_EQ(setting.value(), 10.0);

  std::istringstream iss{"3.145"};
  setting.FromStream(iss);

  EXPECT_EQ(setting.name(), "threshold");
  EXPECT_EQ(setting.description(), "A sample setting");
  EXPECT_DOUBLE_EQ(setting.default_value(), 10.0);
  EXPECT_DOUBLE_EQ(setting.value(), 3.145);
}

TEST(SettingTests, CheckBool)
{
  SettingCollection collection{};
  Setting<bool> setting{collection, "flag", false, "A sample setting"};

  EXPECT_EQ(setting.name(), "flag");
  EXPECT_EQ(setting.description(), "A sample setting");
  EXPECT_EQ(setting.default_value(), false);
  EXPECT_EQ(setting.value(), false);

  for (auto const &on_value : {"true", "1", "on", "enabled"})
  {
    setting.Update(false);

    std::istringstream iss{on_value};
    setting.FromStream(iss);

    EXPECT_EQ(setting.name(), "flag");
    EXPECT_EQ(setting.description(), "A sample setting");
    EXPECT_EQ(setting.default_value(), false);
    EXPECT_EQ(setting.value(), true);
  }

  for (auto const &off_value : {"false", "0", "off", "disabled"})
  {
    setting.Update(true);

    std::istringstream iss{off_value};
    setting.FromStream(iss);

    EXPECT_EQ(setting.name(), "flag");
    EXPECT_EQ(setting.description(), "A sample setting");
    EXPECT_EQ(setting.default_value(), false);
    EXPECT_EQ(setting.value(), false);
  }
}

TEST(SettingTests, CheckStringList)
{
  using StringArray = std::vector<std::string>;

  SettingCollection collection{};
  Setting<StringArray> setting{collection, "peers", {}, "A sample setting"};

  EXPECT_EQ(setting.name(), "peers");
  EXPECT_EQ(setting.description(), "A sample setting");
  EXPECT_EQ(setting.default_value(), StringArray{});
  EXPECT_EQ(setting.value(), StringArray{});

  {
    setting.Update(StringArray{});

    std::istringstream iss{"foo"};
    setting.FromStream(iss);

    EXPECT_EQ(setting.name(), "peers");
    EXPECT_EQ(setting.description(), "A sample setting");
    EXPECT_EQ(setting.default_value(), StringArray{});
    EXPECT_EQ(setting.value(), StringArray({"foo"}));
  }

  {
    setting.Update(StringArray{});

    std::istringstream iss{"foo,bar,baz"};
    setting.FromStream(iss);

    EXPECT_EQ(setting.name(), "peers");
    EXPECT_EQ(setting.description(), "A sample setting");
    EXPECT_EQ(setting.default_value(), StringArray{});
    EXPECT_EQ(setting.value(), StringArray({"foo","bar","baz"}));
  }
}


} // namespace