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

#include <functional>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>

#include "core/commandline/parameter_parser.hpp"
#include "core/macros.hpp"

namespace fetch {
namespace commandline {

class Params
{
public:
  Params(const Params &rhs) = delete;
  Params(Params &&rhs)      = delete;
  Params operator=(const Params &rhs) = delete;
  Params operator=(Params &&rhs)             = delete;
  bool   operator==(const Params &rhs) const = delete;
  bool   operator<(const Params &rhs) const  = delete;

  using action_func_type =
      std::function<void(const std::set<std::string> &, std::list<std::string> &)>;
  using help_text_type  = std::tuple<std::string, std::string>;
  using assigners_type  = std::map<std::string, action_func_type>;
  using help_texts_type = std::list<help_text_type>;

  Params()
    : paramsParser_()
  {}

  void Parse(int argc, char **argv)
  {
    paramsParser_.Parse(argc, argv);

    std::list<std::string> errs;
    std::set<std::string>  args;

    for (int i = 0; i < argc; i++)
    {
      auto s = std::string(argv[i]);
      if (s.size() > 0 && s[0] == '-')
      {
        if (s.size() > 1 && s[1] == '-')
        {
          args.insert(s.substr(2));
        }
        else
        {
          args.insert(s.substr(1));
        }
      }
    }

    for (auto action : assigners_)
    {
      action_func_type func = action.second;
      func(args, errs);
    }

    for (int i = 0; i < argc; i++)
    {
      std::string arg(argv[i]);
      if (arg == "--help" || arg == "-h")
      {
        help();
        exit(0);
      }
    }

    if (!errs.empty())
    {
      for (auto err : errs)
      {
        std::cerr << err << std::endl;
      }
      exit(1);
    }
  }

  virtual ~Params()
  {}

  template <class TYPE>
  void add(TYPE &assignee, const std::string &name, const std::string &help, TYPE deflt)
  {
    std::string name_local(name);
    TYPE        deflt_local = deflt;
    assigners_[name]        = [name_local, &assignee, deflt_local, this](
                           const std::set<std::string> &args,
                           std::list<std::string> &     errs) mutable {
      FETCH_UNUSED(args);
      FETCH_UNUSED(errs);

      assignee = this->paramsParser_.GetParam(name_local, deflt_local);
    };

    helpTexts_.push_back(help_text_type{name, help});
  }

  template <class TYPE>
  void add(TYPE &assignee, const std::string &name, const std::string &help)
  {
    std::string name_local(name);
    assigners_[name] = [name_local, &assignee, this](const std::set<std::string> &args,
                                                     std::list<std::string> &     errs) mutable {
      if (args.find(name_local) == args.end())
      {
        errs.push_back("Missing required argument: " + name_local);
        return;
      }
      assignee = this->paramsParser_.GetParam(name_local, TYPE());
    };

    helpTexts_.push_back(help_text_type{name, help});
  }

  void description(const std::string &desc)
  {
    desc_ = desc;
  }

  void help()
  {
    if (!desc_.empty())
    {
      std::cerr << desc_ << '\n' << std::endl;
    }

    std::size_t w = 0;

    for (auto helpText : helpTexts_)
    {
      auto length = std::get<0>(helpText).length();
      if (length > w)
      {
        w = length;
      }
    }

    for (auto helpText : helpTexts_)
    {
      auto padsize = w - std::get<0>(helpText).length();
      std::cerr << "  -" << std::get<0>(helpText) << std::string(2 + padsize, ' ')
                << std::get<1>(helpText) << std::endl;
    }
  }

private:
  fetch::commandline::ParamsParser paramsParser_;
  std::string                      desc_;
  help_texts_type                  helpTexts_;
  assigners_type                   assigners_;
};

}  // namespace commandline
}  // namespace fetch
