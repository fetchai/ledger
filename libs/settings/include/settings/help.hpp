#pragma once
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

#include "settings/detail/csv_string_helpers.hpp"
#include "settings/setting_base.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace fetch {
namespace settings {

class SettingCollection;

class Help : public SettingBase
{
public:
  // Construction / Destruction
  explicit Help(SettingCollection &reg);

  Help(Help const &) = delete;
  Help(Help &&)      = delete;
  ~Help() override   = default;

  Help &operator=(Help const &) = delete;
  Help &operator=(Help &&) = delete;

  void FromStream(std::istream &stream) override;
  void ToStream(std::ostream &stream) const override;

  bool        TerminateNow() const override;
  std::string envname() const override;

private:
  SettingCollection &reg_;
};

}  // namespace settings
}  // namespace fetch
