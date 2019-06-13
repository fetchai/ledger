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

#include "core/commandline/parameter_parser.hpp"

namespace fetch {
namespace commandline {

void ParamsParser::Parse(int argc, char **argv)
{
  arg_count_        = static_cast<std::size_t>(argc);
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

std::string ParamsParser::GetArg(std::size_t const &i) const
{
  if (i >= args_.size())
  {
    throw std::runtime_error("parameter does not exist");
  }

  return args_[i];
}

std::string ParamsParser::GetArg(std::size_t const &i, std::string const &default_value) const
{
  if (i >= args_.size())
  {
    return default_value;
  }

  return args_[i];
}

std::string ParamsParser::GetParam(std::string const &key, std::string const &default_value) const
{
  if (params_.find(key) == params_.end())
  {
    return default_value;
  }

  return params_.find(key)->second;
}

}  // namespace commandline
}  // namespace fetch
