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

#include "vm_modules/core/system.hpp"

fetch::vm_modules::Parameters fetch::vm_modules::System::params{};

void fetch::vm_modules::System::Bind(fetch::vm::Module &module)
{
  module.CreateClassType<System>("System")
      .CreateStaticMemberFunction("Argc", &System::Argc)
      .CreateStaticMemberFunction("Argv", &System::Argv);
}

int32_t fetch::vm_modules::System::Argc(fetch::vm::VM * /*vm*/, fetch::vm::TypeId /*type_id*/)
{
  return static_cast<int32_t>(params.script().size());
}

fetch::vm::Ptr<fetch::vm::String> fetch::vm_modules::System::Argv(fetch::vm::VM *vm,
                                                                  fetch::vm::TypeId /*type_id*/,
                                                                  int32_t index)
{
  return fetch::vm::Ptr<fetch::vm::String>{
      new fetch::vm::String{vm, params.script().at(static_cast<std::size_t>(index))}};
}

void fetch::vm_modules::System::Parse(int argc, char const *const *argv)
{
  params.Parse(argc, argv);
}

const fetch::vm_modules::Parameters::ParamsParser &fetch::vm_modules::System::GetParamsParser()
{
  return params.program();
}

void fetch::vm_modules::Parameters::Parse(int argc, char const *const argv[])
{
  script_args_.clear();

  const std::string SEPARATOR{"--"};

  ArgList program_args{};
  ArgList script_args{};

  // first parameter is common between both argument set
  program_args.push_back(argv[0]);
  script_args.push_back(argv[0]);

  // loop through all the parameters
  ArgList *current_list = &program_args;
  for (int i = 1; i < argc; ++i)
  {
    if (SEPARATOR == argv[i])
    {
      // switch the list
      current_list = &script_args;
    }
    else
    {
      // add the string to the list
      current_list->push_back(argv[i]);
    }
  }

  // parse the program arguments
  program_params_.Parse(static_cast<int>(program_args.size()), program_args.data());

  // copy the script arguments
  for (auto const *s : script_args)
  {
    script_args_.emplace_back(s);
  }
}

const fetch::vm_modules::Parameters::ParamsParser &fetch::vm_modules::Parameters::program() const
{
  return program_params_;
}

const fetch::vm_modules::Parameters::StringList &fetch::vm_modules::Parameters::script() const
{
  return script_args_;
}
