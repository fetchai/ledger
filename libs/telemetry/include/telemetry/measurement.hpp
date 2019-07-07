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

  enum class StreamMode
  {
    FULL,
    WITHOUT_HEADER,
  };

  // Construction / Destruction
  Measurement(std::string name, std::string description, Labels labels = Labels{});
  virtual ~Measurement() = default;

  /// @name Accessors
  /// @{
  std::string const &name() const;
  std::string const &description() const;
  Labels const &labels() const;
  /// @}

  /// @name Metric Interface
  /// @{

  /**
   * Write the value of the metric to the stream so as to be consumed by external components
   *
   * @param stream The stream to be updated
   * @param mode The mode to be used when generating the stream
   */
  virtual void ToStream(std::ostream &stream, StreamMode mode) const = 0;

  /// @}

protected:

  std::ostream &WriteHeader(std::ostream &stream, char const *type_name, StreamMode mode) const;
  std::ostream &WriteValuePrefix(std::ostream &stream) const;
  std::ostream &WriteValuePrefix(std::ostream &stream, std::string const &suffix) const;
  std::ostream &WriteValuePrefix(std::ostream &stream, std::string const &suffix, Labels const &extra) const;

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

inline Measurement::Labels const &Measurement::labels() const
{
  return labels_;
}

}  // namespace telemetry
}  // namespace fetch

namespace std
{

template <>
struct hash<fetch::telemetry::Measurement::Labels>
{
  std::size_t operator()(fetch::telemetry::Measurement::Labels const &labels) const
  {
    std::size_t value{0};

    if (!labels.empty())
    {
      std::hash<std::string> const hasher{};

      auto it = labels.begin();

      // first element populate the hash
      value = (hasher(it->first) ^ hasher(it->second));
      ++it;

      for (auto end = labels.end(); it != end; ++it)
      {
        value ^= (hasher(it->first) ^ hasher(it->second));
      }
    }

    return value;
  }
};

}
