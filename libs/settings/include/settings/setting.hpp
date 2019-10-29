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

#include "settings/detail/csv_string_helpers.hpp"
#include "settings/setting_base.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace fetch {
namespace settings {

class SettingCollection;

template <typename T>
class Setting : public SettingBase
{
public:
  // Construction / Destruction
  Setting(SettingCollection &reg, std::string name, T const &default_value,
          std::string description);
  Setting(SettingCollection &reg, std::string name, T &&default_value, std::string description);
  Setting(Setting const &) = delete;
  Setting(Setting &&)      = delete;
  ~Setting() override      = default;

  /// @name Accessors
  /// @{
  T const &value() const;
  T const &default_value() const;
  /// @}

  /// @name Update
  /// @{
  void Update(T const &value);
  void Update(T &&value);
  /// @}

  /// @name Stream Operations
  /// @{
  void FromStream(std::istream &stream) override;
  void ToStream(std::ostream &stream) const override;
  /// @}

  // Operators
  Setting &operator=(Setting const &) = delete;
  Setting &operator=(Setting &&) = delete;

private:
  T default_{};
  T value_{default_};
};

template <typename T>
Setting<T>::Setting(SettingCollection &reg, std::string name, T const &default_value,
                    std::string description)
  : SettingBase{reg, std::move(name), std::move(description)}
  , default_{default_value}
{}

template <typename T>
Setting<T>::Setting(SettingCollection &reg, std::string name, T &&default_value,
                    std::string description)
  : SettingBase{reg, std::move(name), std::move(description)}
  , default_{std::move(default_value)}
{}

template <typename T>
T const &Setting<T>::value() const
{
  return value_;
}

template <typename T>
T const &Setting<T>::default_value() const
{
  return default_;
}

template <typename T>
void Setting<T>::Update(T const &value)
{
  value_ = value;
}

template <typename T>
void Setting<T>::Update(T &&value)
{
  value_ = std::move(value);
}

template <typename T>
void Setting<T>::FromStream(std::istream &stream)
{
  auto const original = value_;

  stream >> value_;

  if (stream.fail())
  {
    value_ = original;
  }
}

template <typename T>
void Setting<T>::ToStream(std::ostream &stream) const
{
  stream << value_;
}

template <>
void Setting<bool>::FromStream(std::istream &stream);

template <>
void Setting<bool>::ToStream(std::ostream &stream) const;

}  // namespace settings
}  // namespace fetch
