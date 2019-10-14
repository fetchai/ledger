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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <iostream>
using fetch::byte_array::ConstByteArray;
using namespace fetch::vm;
using namespace fetch;
using fetch::vm_modules::VMFactory;

constexpr auto SCRIPT1 = R"(
function SomeRandomFunction(x: Array< Int32 >, y: Map< Int32, String >) : Array< Int32 >
  var ret = Array< Int32 >( x.count() );
  for(i in 0:x.count())
    printLn(y[x[i]]);
    ret[i] = 2i32 * x[i];
  endfor

  return ret;
endfunction


function Test(s: String)
  print("Welcome to Etch:");
  printLn(s);
endfunction

function Test2(i: Int32, j: UInt64, s: String)
  printLn(i);
  printLn(j);
  print("Was here: ");
  printLn(s);
endfunction

function Test3(arr: Array< Float64>)
  printLn(arr.count());
  for(i in 0:arr.count())
    printLn(arr[i]);
  endfor
endfunction
)";

struct ExecutionTask
{
  std::string    function;
  ConstByteArray parameters;

  bool DeserializeParameters(vm::VM *vm, ParameterPack &params, Executable::Function const *f) const
  {
    // Finding the relevant function in the executable
    if (f == nullptr)
    {
      return false;
    }

    // Preparing serializer and return value.
    MsgPackSerializer serializer{parameters};

    // Extracting parameters
    auto const num_parameters = static_cast<std::size_t>(f->num_parameters);

    // loop through the parameters, type check and populate the stack
    for (std::size_t i = 0; i < num_parameters; ++i)
    {
      auto type_id = f->variables[i].type_id;
      if (type_id <= vm::TypeIds::PrimitiveMaxId)
      {
        Variant param;
        serializer >> param;
        params.AddSingle(param);
      }
      else
      {
        // Checking if we can construct the object
        if (!vm->IsDefaultSerializeConstructable(type_id))
        {
          return false;
        }

        // Creating the object
        vm::Ptr<vm::Object> object  = vm->DefaultSerializeConstruct(type_id);
        auto                success = object->DeserializeFrom(serializer);

        // If deserialization failed we return
        if (!success)
        {
          return false;
        }

        // Adding the parameter to the parameter pack
        params.AddSingle(object);
      }
    }

    return true;
  }

  void SerializeParameters(ParameterPack &params)
  {
    // Serializing the parameters
    MsgPackSerializer serializer;
    for (std::size_t i = 0; i < params.size(); ++i)
    {
      auto &param = params[i];
      if (param.type_id <= vm::TypeIds::PrimitiveMaxId)
      {
        serializer << param;
      }
      else
      {
        param.object->SerializeTo(serializer);
      }
    }

    // Storing the result
    parameters = serializer.data();
  }
};

ConstByteArray CreateVMAndRunScript(std::string script, ExecutionTask const &task)
{
  fetch::vm::SourceFiles files;
  files.push_back(fetch::vm::SourceFile("hello.etch", script));

  auto executable = std::make_unique<Executable>();
  auto module     = VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS);

  // attempt to compile the program
  auto errors = VMFactory::Compile(module, files, *executable);

  // detect compilation errors
  if (!errors.empty())
  {
    std::cerr << "Failed to compile:\n";

    for (auto const &line : errors)
    {
      std::cerr << line << '\n';
    }

    return "";
  }

  // create the VM instance
  auto vm = std::make_unique<VM>(module.get());
  vm->AttachOutputDevice(VM::STDOUT, std::cout);

  // Execute the requested function
  std::string error;
  std::string console;
  Variant     output;

  // Unpacking arguments
  ParameterPack params{vm->registered_types()};
  if (!task.DeserializeParameters(vm.get(), params, executable->FindFunction(task.function)))
  {
    std::cerr << "Failed to deserialize parameters" << std::endl;
    return "";
  }

  // Executing
  bool const success = vm->Execute(*executable, task.function, error, output, params);

  if (!success)
  {
    std::cerr << error << std::endl;
    return "";
  }
  // if there is any console output print it
  if (!console.empty())
  {
    std::cout << console << std::endl;
  }

  return "";
}

template <typename... Args>
void SetInputParameters(ExecutionTask &task, Args... args)
{
  // Creating module for packing parameters. This is needed to generate
  // the unique type identifiers.
  auto module = VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS);

  // TODO: Needed to ensure that types are registered.
  auto compiler = std::make_shared<Compiler>(module.get());

  // ParameterPack params{module->registered_types()};
  // TODO: Preferably we would get rid of the VM here
  auto          vm = std::make_unique<VM>(module.get());
  ParameterPack params{vm->registered_types(), vm.get()};

  // Adding parameters
  if (!params.Add(args...))
  {
    throw std::runtime_error("could not pack parameters");
  }

  // Serializing the parameters
  task.SerializeParameters(params);
}

void TestA()
{
  // Creating execution task
  ExecutionTask task;
  task.function = "Test";
  SetInputParameters(task, static_cast<std::string>("hello world"));

  // Executing task
  CreateVMAndRunScript(SCRIPT1, task);
}

void TestB()
{
  // Creating execution task
  ExecutionTask task;
  task.function = "Test2";
  SetInputParameters(task, int32_t(2), uint64_t(9), static_cast<std::string>("hello world"));

  // Executing task
  CreateVMAndRunScript(SCRIPT1, task);
}

int main()
{
  // Creating execution task
  ExecutionTask task;
  task.function = "Test3";
  std::vector<double> xxx{9, 2, 3, 4};
  SetInputParameters(task, xxx);

  // Executing task
  CreateVMAndRunScript(SCRIPT1, task);
  return 0;
}
