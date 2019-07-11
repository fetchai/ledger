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

#include <cstddef>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace commandline {

class ParamsParser
{
public:
  // Construction / Destruction
  ParamsParser()                     = default;
  ParamsParser(ParamsParser const &) = delete;
  ParamsParser(ParamsParser &&)      = delete;
  ~ParamsParser()                    = default;

  void Parse(int argc, char **argv);

  template <typename T>
  T GetArg(std::size_t const &i, T const &default_value) const;
  template <typename T>
  T           GetArg(std::size_t const &i) const;
  std::string GetArg(std::size_t const &i) const;
  std::string GetArg(std::size_t const &i, std::string const &default_value) const;

  template <typename T>
  T           GetParam(std::string const &key, T const &default_value) const;
  bool        LookupParam(std::string const &key, std::string &value) const;

  std::string GetParam(std::string const &key, std::string const &default_value) const;

  std::size_t arg_size() const;

  // Operators
  ParamsParser &operator=(ParamsParser const &) = delete;
  ParamsParser &operator=(ParamsParser &&) = delete;

private:
  std::map<std::string, std::string> params_{};
  std::vector<std::string>           args_{};
  std::size_t                        arg_count_{0};
};

template <typename T>
T ParamsParser::GetArg(std::size_t const &i, T const &default_value) const
{
  if (i >= args_.size())
  {
    return default_value;
  }

  std::stringstream s(args_[i]);
  T                 ret;
  s >> ret;
  return ret;
}

template <typename T>
T ParamsParser::GetArg(std::size_t const &i) const
{
  if (i >= args_.size())
  {
    throw std::runtime_error("parameter does not exist");
  }

  std::stringstream s(args_[i]);
  T                 ret;
  s >> ret;
  return ret;
}

template <typename T>
T ParamsParser::GetParam(std::string const &key, T const &default_value) const
{
  if (params_.find(key) == params_.end())
  {
    return default_value;
  }

  std::stringstream s(params_.find(key)->second);
  T                 ret;
  s >> ret;
  return ret;
}

inline std::size_t ParamsParser::arg_size() const
{
  return args_.size();
}

}  // namespace commandline
}  // namespace fetch
