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

#include "core/commandline/parameter_parser.hpp"
#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {

class Parameters {
public:
  using ParamParser = fetch::commandline::ParamsParser;
  using ArgList     = std::vector<char *>;
  using StringList  = std::vector<std::string>;

  void Parse(int argc, char **argv) {
    const std::string SEPARATOR{"--"};

    ArgList program_args{};
    ArgList script_args{};

    // first parameter is common between both argument set
    program_args.push_back(argv[0]);
    script_args.push_back(argv[0]);

    // loop through all the parameters
    ArgList *current_list = &program_args;
    for (int i = 1; i < argc; ++i) {
      if (SEPARATOR == argv[i]) {
        // switch the list
        current_list = &script_args;
      } else {
        // add the string to the list
        current_list->push_back(argv[i]);
      }
    }

    // parse the program arguments
    program_params_.Parse(static_cast<int>(program_args.size()), program_args.data());

    // copy the script arguments
    for (auto const *s : script_args) {
      script_args_.emplace_back(s);
    }
  }

  ParamParser const &program() const {
    return program_params_;
  }

  StringList const &script() const {
    return script_args_;
  }

private:
  ParamParser program_params_{};
  StringList script_args_{};
};

struct System : public fetch::vm::Object {
  System()           = delete;
  ~System() override = default;

  static void Bind(fetch::vm::Module & module) {
    module.CreateClassType<System>("System")
        .CreateStaticMemberFunction("Argc", &System::Argc)
        .CreateStaticMemberFunction("Argv", &System::Argv);
  }

  static int32_t Argc(fetch::vm::VM *, fetch::vm::TypeId) {
    return static_cast<int32_t>(params.script().size());
  }

  static fetch::vm::Ptr <fetch::vm::String> Argv(fetch::vm::VM *vm, fetch::vm::TypeId, int32_t index) {
    return {new fetch::vm::String{vm, params.script().at(static_cast<std::size_t>(index))}};
  }

  static void Parse(int argc, char **argv) {
    params.Parse(argc, argv);
  }

  static Parameters::ParamParser const & GetParamParser(){
    return params.program();
  }

private:
  static Parameters params;
};

}
}