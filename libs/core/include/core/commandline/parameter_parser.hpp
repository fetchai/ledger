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

#include <algorithm>
#include <exception>
#include <map>
#include <sstream>
#include <vector>
namespace fetch {
namespace commandline {

class ParamsParser
{
private:
  std::map<std::string, std::string> params_;
  std::vector<std::string>           args_;
  int                                arg_count_;

public:
  void Parse(int argc, char **argv)
  {
    arg_count_        = argc;
    std::size_t sargs = std::size_t(argc);
    for (std::size_t i = 0; i < sargs; ++i)
    {
      std::string name(argv[i]);
      if (name.find("-") == 0)
      {
        name = name.substr(1);
        ++i;
        if (i == sargs)
        {
          params_[name] = "1";
          continue;
        }

        std::string value(argv[i]);

        if (value.find("-") == 0)
        {
          params_[name] = "1";
          --i;
        }
        else
        {
          params_[name] = value;
        }
      }
      else
      {
        args_.push_back(name);
      }
    }
  }

  template <typename T>
  T GetArg(std::size_t const &i, T const &default_value) const
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
  T GetArg(std::size_t const &i) const
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

  std::string GetArg(std::size_t const &i) const
  {
    if (i >= args_.size())
    {
      throw std::runtime_error("parameter does not exist");
    }

    return args_[i];
  }

  std::string GetArg(std::size_t const &i, std::string const &default_value) const
  {
    if (i >= args_.size())
    {
      return default_value;
    }

    return args_[i];
  }

  std::string GetParam(std::string const &key, std::string const &default_value) const
  {
    if (params_.find(key) == params_.end())
    {
      return default_value;
    }

    return params_.find(key)->second;
  }

  template <typename T>
  T GetParam(std::string const &key, T const &default_value) const
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

  std::size_t arg_size() const
  {
    return args_.size();
  }
};
}  // namespace commandline
}  // namespace fetch
