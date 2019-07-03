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
#include <unordered_map>

namespace fetch {
namespace telemetry {

class Measurement
{
public:
  using Labels = std::unordered_map<std::string, std::string>;

  // Construction / Destruction
  Measurement(std::string name, std::string description, Labels labels = Labels{});
  virtual ~Measurement() = default;

  /// @name Accessors
  /// @{
  std::string const &name() const;
  std::string const &description() const;
  /// @}

  /// @name Metric Interface
  /// @{
  virtual void ToStream(std::ostream &stream) const = 0;
  /// @}

protected:
  std::ostream &WritePrefix(std::ostream &stream, char const *type_name) const;

private:
  std::string const name_;
  std::string const description_;
  Labels            labels_;
};

inline Measurement::Measurement(std::string name, std::string description, Labels labels)
  : name_{std::move(name)}
  , description_{std::move(description)}
  , labels_{std::move(labels)}
{}

inline std::string const &Measurement::name() const
{
  return name_;
}

inline std::string const &Measurement::description() const
{
  return description_;
}

}  // namespace telemetry
}  // namespace fetch
