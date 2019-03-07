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

#include "core/commandline/cli_header.hpp"
#include "core/commandline/params.hpp"
#include "vm/analyser.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/typeids.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <fstream>
#include <memory>
#include <sstream>
#include <streambuf>
#include <vector>

namespace {

using fetch::vm::Script;
using fetch::vm::Ptr;
using fetch::vm::VM;
using fetch::vm::String;
using fetch::vm::TypeId;
using fetch::vm_modules::VMFactory;

class Parameters
{
public:
  using ParamParser = fetch::commandline::ParamsParser;
  using ArgList     = std::vector<char *>;
  using StringList  = std::vector<std::string>;

  void Parse(int argc, char **argv)
  {
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

  ParamParser const &program() const
  {
    return program_params_;
  }

  StringList const &script() const
  {
    return script_args_;
  }

private:
  ParamParser program_params_{};
  StringList  script_args_{};
};

std::string ReadFileContents(std::string const &path)
{
  std::ifstream f(path.c_str());
  std::string   str{};

  // pre-allocate the string buffer
  f.seekg(0, std::ios::end);
  str.reserve(static_cast<std::size_t>(f.tellg()));
  f.seekg(0, std::ios::beg);

  // assign the contents
  str.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

  return str;
}

// data
Parameters params;

int32_t Argc(VM *, TypeId)
{
  return static_cast<int32_t>(params.script().size());
}

Ptr<String> Argv(VM *vm, TypeId, int32_t index)
{
  return {new String{vm, params.script().at(static_cast<std::size_t>(index))}};
}

// placeholder class
struct System : public fetch::vm::Object
{
};

}  // namespace

int main(int argc, char **argv)
{
  // parse the command line parameters
  params.Parse(argc, argv);

  // ensure the program has the correct number of args
  if (2u != params.program().arg_size())
  {
    std::cerr << "Usage: " << argv[0] << " [options] <filename> -- [script args]..." << std::endl;
    return 1;
  }

  // print the header
  fetch::commandline::DisplayCLIHeader("vm-lang");

  // load the contents of the script file
  auto const source = ReadFileContents(params.program().GetArg(1));

  auto script = std::make_unique<Script>();
  auto module = VMFactory::GetModule();

  // additional module bindings
  module->CreateClassType<System>("System")
      .CreateTypeFunction("Argc", &Argc)
      .CreateTypeFunction("Argv", &Argv);

  // attempt to compile the program
  auto errors = VMFactory::Compile(module, source, *script);

  // detect compilation errors
  if (!errors.empty())
  {
    std::cerr << "Failed to compile:\n";

    for (auto const &line : errors)
    {
      std::cerr << line << '\n';
    }

    return 1;
  }

  // create the VM instance
  auto vm = VMFactory::GetVM(module);

  // Execute the requested function
  std::string        error;
  std::string        console;
  fetch::vm::Variant output;
  bool const         success =
      vm->Execute(*script, params.program().GetParam("func", "main"), error, console, output);

  // if there is any console output print it
  if (!console.empty())
  {
    std::cout << console << std::endl;
  }

  return (success) ? 0 : 1;
}
