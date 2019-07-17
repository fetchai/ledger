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

#include <iosfwd>
#include <string>

namespace fetch {
namespace settings {

class SettingCollection;

class SettingBase
{
public:
  // Construction / Destruction
  SettingBase(SettingBase const &) = delete;
  SettingBase(SettingBase &&)      = delete;
  virtual ~SettingBase()           = default;

  /// @name Accessors
  /// @{
  std::string const &name() const;
  std::string const &description() const;
  /// @}

  /// @name Stream operations
  /// @{
  virtual void FromStream(std::istream &stream)     = 0;
  virtual void ToStream(std::ostream &stream) const = 0;
  /// @}

  // Operators
  SettingBase &operator=(SettingBase const &) = delete;
  SettingBase &operator=(SettingBase &&) = delete;

protected:
  SettingBase(SettingCollection &reg, std::string &&name, std::string &&description);

private:
  std::string name_{};
  std::string description_{};
};

inline std::istream &operator>>(std::istream &stream, SettingBase &setting)
{
  setting.FromStream(stream);
  return stream;
}

inline std::ostream &operator<<(std::ostream &stream, SettingBase const &setting)
{
  setting.ToStream(stream);
  return stream;
}

}  // namespace settings
}  // namespace fetch
