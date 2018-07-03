#ifndef PARAMS_HPP
#define PARAMS_HPP

#include <functional>
#include <iostream>
#include <string>
#include <list>
#include <tuple>
#include <set>
#include <stdexcept>

#include "core/commandline/parameter_parser.hpp"

namespace fetch {
namespace commandline {

class Params
{
public:
  Params(const Params &rhs)           = delete;
  Params(Params &&rhs)           = delete;
  Params operator=(const Params &rhs)  = delete;
  Params operator=(Params &&rhs) = delete;
  bool operator==(const Params &rhs) const = delete;
  bool operator<(const Params &rhs) const = delete;

  fetch::commandline::ParamsParser paramsParser_;

  std::string desc_;

  typedef std::function<void(const std::set<std::string> &, std::list<std::string> &)> ACTION_FUNCTION;

  typedef std::tuple<std::string, std::string> HELP_TEXT;

  typedef std::map<std::string, ACTION_FUNCTION> ASSIGNERS;

  typedef std::list<HELP_TEXT> HELP_TEXTS;

  HELP_TEXTS helpTexts_;
  ASSIGNERS assigners_;

  Params():paramsParser_()
  {
  }

  void Parse(int argc, const char *argv[])
  {
    paramsParser_.Parse(argc, argv);

    std::list<std::string> errs;
    std::set<std::string> args;

    for(int i=0;i<argc;i++)
      {
        auto s = std::string(argv[i]);
        if (s[0] == '-')
          {
            if (s[1] == '-')
              {
                args.insert(s.substr(2));
              }
            else
              {
                args.insert(s.substr(1));
              }
          }
      }

    for(auto action : assigners_)
      {
        ACTION_FUNCTION func = action.second;
        func(args, errs);
      }

    for(int i = 0;i< argc;i++)
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
        for(auto err : errs)
          {
            std::cerr << err << std::endl;
          }
        exit(1);
      }
  }


  virtual ~Params()
  {
  }

  template<class TYPE>
  void add(TYPE &assignee, const std::string &name, const std::string &help, TYPE deflt)
  {
    std::string name_local(name);
    TYPE deflt_local = deflt;
    assigners_[name] = [name_local,assignee,deflt_local,this](const std::set<std::string> &args, std::list<std::string> &errs) mutable {
      assignee = this -> paramsParser_.GetParam<TYPE>(name_local, deflt_local);
    };

    helpTexts_.push_back(HELP_TEXT(name, help));
  }

  template<class TYPE>
  void add(TYPE &assignee, const std::string &name, const std::string &help)
  {
    std::string name_local(name);
    assigners_[name] =
      [name_local,&assignee,this](const std::set<std::string> &args, std::list<std::string> &errs) mutable {
      if (args.find(name_local) == args.end())
        {
          errs.push_back(std::string("missingarg:") + name_local);
          return;
        }

      //std::cout << "Assigning " < assignee << " to " << name_local << " is " <<  this->paramsParser_.GetParam<TYPE>(name_local, TYPE()) << std::endl;
      assignee = this -> paramsParser_.GetParam<TYPE>(name_local, TYPE());
    };

    helpTexts_.push_back(HELP_TEXT(name, help));
  }

  void description(const std::string &desc)
  {
    desc_ = desc;
  }

  void help()
  {
    std::cerr << desc_ << std::endl;
    std::cerr << std::endl;

    size_t w = 0;

    for(auto helpText : helpTexts_)
      {
        auto length = std::get<0>(helpText).length();
        if (length> w)
          {
            w = length;
          }
      }

    for(auto helpText : helpTexts_)
      {
        std::cerr << "  " << std::get<0>(helpText) << std::string( 2 + w - std::get<0>(helpText).length(), ' ') << std::get<1>(helpText) << std::endl;
      }
  }
};

}
}

#endif //__PARAMS_HPP
