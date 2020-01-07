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

#include "settings/setting_base.hpp"
#include "settings/setting_collection.hpp"

#include <istream>
#include <ostream>
#include <string>
#include <utility>

namespace fetch {
namespace settings {

SettingBase::SettingBase(SettingCollection &reg, std::string &&name, std::string &&description)
  : name_{std::move(name)}
  , description_{std::move(description)}
{
  reg.Add(*this);
}

std::string const &SettingBase::name() const
{
  return name_;
}

std::string const &SettingBase::description() const
{
  return description_;
}

std::istream &operator>>(std::istream &stream, SettingBase &setting)
{
  setting.FromStream(stream);
  return stream;
}

std::ostream &operator<<(std::ostream &stream, SettingBase const &setting)
{
  setting.ToStream(stream);
  return stream;
}

}  // namespace settings
}  // namespace fetch
