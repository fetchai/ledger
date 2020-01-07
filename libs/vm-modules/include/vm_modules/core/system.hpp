#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

class Parameters
{
public:
  using ParamsParser = fetch::commandline::ParamsParser;
  using ArgList      = std::vector<char const *>;
  using StringList   = std::vector<std::string>;

  void Parse(int argc, char const *const argv[]);

  ParamsParser const &program() const;

  StringList const &script() const;

private:
  ParamsParser program_params_;
  StringList   script_args_;
};

struct System : public fetch::vm::Object
{
  System()           = delete;
  ~System() override = default;

  static void Bind(fetch::vm::Module &module);

  static int32_t Argc(fetch::vm::VM * /*vm*/, fetch::vm::TypeId /*type_id*/);

  static fetch::vm::Ptr<fetch::vm::String> Argv(fetch::vm::VM *vm, fetch::vm::TypeId /*type_id*/,
                                                int32_t        index);

  static void Parse(int argc, char const *const *argv);

  static Parameters::ParamsParser const &GetParamsParser();

private:
  static Parameters params;
};

}  // namespace vm_modules
}  // namespace fetch
