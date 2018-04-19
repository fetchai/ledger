#ifndef COMMANDLINE_PARAMETER_PARSER_HPP
#define COMMANDLINE_PARAMETER_PARSER_HPP

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>
#include<exception>
namespace fetch {
namespace commandline {

class ParamsParser {
 private:
  std::map<std::string, std::string> params_;
  std::vector<std::string> args_;
  int arg_count_;

 public:
  void Parse(int argc, char const **argv) {
    arg_count_ = argc;
    for (std::size_t i = 0; i < argc; ++i) {
      std::string name(argv[i]);
      if (name.find("-") == 0) {
        name = name.substr(1);
        ++i;
        if (i == argc) {
          params_[name] = "1";
          continue;
        }

        std::string value(argv[i]);

        if (value.find("-") == 0) {
          params_[name] = "1";
          --i;
        } else
          params_[name] = value;

      } else
        args_.push_back(name);
    }
  }

  template <typename T>
  T GetArg(std::size_t const &i, T const &default_value) const {
    if (i >= args_.size()) return default_value;

    std::stringstream s(args_[i]);
    T ret;
    s >> ret;
    return ret;
  }

  template <typename T>
  T GetArg(std::size_t const &i) const {
    if (i >= args_.size()) throw std::runtime_error("parameter does not exist");    

    std::stringstream s(args_[i]);
    T ret;
    s >> ret;
    return ret;
  }

  
  std::string GetArg(std::size_t const &i) const {
    if (i >= args_.size()) throw std::runtime_error("parameter does not exist");

    return args_[i];
  }


  std::string GetArg(std::size_t const &i,
                     std::string const &default_value) const {
    if (i >= args_.size()) return default_value;

    return args_[i];
  }

  std::string GetParam(std::string const &key,
                       std::string const &default_value) const {
    if (params_.find(key) == params_.end()) return default_value;

    return params_.find(key)->second;
  }

  template <typename T>
  T GetParam(std::string const &key, T const &default_value) const {
    if (params_.find(key) == params_.end()) return default_value;

    std::stringstream s(params_.find(key)->second);
    T ret;
    s >> ret;
    return ret;
  }
  
  std::size_t arg_size() const { return args_.size(); } 
};
};
};
#endif
