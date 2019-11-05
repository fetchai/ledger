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

#include "core/byte_array/byte_array.hpp"
#include "dmlf/execution/basic_vm_engine.hpp"
#include "dmlf/var_converter.hpp"

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/common.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <memory>
#include <sstream>

namespace fetch {
namespace dmlf {

// This must match the type as defined in variant::variant.hpp
using fp64_t = fixed_point::fp64_t;
using fp32_t = fixed_point::fp32_t;

ExecutionResult BasicVmEngine::CreateExecutable(Name const &execName, SourceFiles const &sources)
{
  if (HasExecutable(execName))
  {
    return EngineError(Error::Code::BAD_EXECUTABLE, "executable " + execName + " already exists.");
  }

  auto newExecutable = std::make_shared<Executable>();
  auto errors        = fetch::vm_modules::VMFactory::Compile(module_, sources, *newExecutable);

  if (!errors.empty())
  {
    std::stringstream errorString;
    for (auto const &line : errors)
    {
      errorString << line << '\n';
    }

    return ExecutionResult{
        LedgerVariant{},
        Error{Error::Stage::COMPILE, Error::Code::COMPILATION_ERROR, errorString.str()},
        std::string{}};
  }
  executables_.emplace(execName, std::move(newExecutable));

  return ExecutionResult{
      LedgerVariant(),
      Error{Error::Stage::COMPILE, Error::Code::SUCCESS, "Created executable " + execName},
      std::string{}};
}

ExecutionResult BasicVmEngine::DeleteExecutable(Name const &execName)
{
  auto it = executables_.find(execName);

  if (it == executables_.end())
  {
    return EngineError(Error::Code::BAD_EXECUTABLE, "executable " + execName + " does not exist.");
  }

  executables_.erase(it);
  return EngineSuccess("Deleted executable " + execName);
}

ExecutionResult BasicVmEngine::CreateState(Name const &stateName)
{
  if (HasState(stateName))
  {
    return EngineError(Error::Code::BAD_STATE, "state " + stateName + " already exists.");
  }

  states_.emplace(stateName, std::make_shared<State>());
  return ExecutionResult{
      LedgerVariant{},
      Error{Error::Stage::ENGINE, Error::Code::SUCCESS, "Created state " + stateName},
      std::string{}};
}

ExecutionResult BasicVmEngine::CopyState(Name const &srcName, Name const &newName)
{
  if (!HasState(srcName))
  {
    return EngineError(Error::Code::BAD_STATE, "No state named " + srcName);
  }
  if (HasState(newName))
  {
    return EngineError(Error::Code::BAD_DESTINATION, "state " + newName + " already exists.");
  }

  states_.emplace(newName, std::make_shared<State>(states_[srcName]->DeepCopy()));
  return EngineSuccess("Copied state " + srcName + " to " + newName);
}

ExecutionResult BasicVmEngine::DeleteState(Name const &stateName)
{
  auto it = states_.find(stateName);
  if (it == states_.end())
  {
    return EngineError(Error::Code::BAD_STATE, "No state named " + stateName);
  }

  states_.erase(it);
  return EngineSuccess("Deleted state " + stateName);
}

ExecutionResult BasicVmEngine::Run(Name const &execName, Name const &stateName,
                                   std::string const &entrypoint, Params params)
{
  if (!HasExecutable(execName))
  {
    return EngineError(Error::Code::BAD_EXECUTABLE, "Error: No executable " + execName);
  }
  if (!HasState(stateName))
  {
    return EngineError(Error::Code::BAD_STATE, "Error: No state " + stateName);
  }

  auto &exec  = executables_[execName];
  auto &state = states_[stateName];
  auto const *func = exec->FindFunction(entrypoint);
  if (func == nullptr)
  {
    return EngineError(Error::Code::RUNTIME_ERROR, entrypoint + " does not exist");
  }

  // We create a a VM for each execution. It might be better to create a single VM and reuse it, but
  // (currently) if you create a VM before compiling the VM is badly formed and crashes on execution
  VM vm{module_.get()};
  vm.SetIOObserver(*state);
  std::ostringstream console{};
  vm.AttachOutputDevice(fetch::vm::VM::STDOUT, console);
  vm::ParameterPack parameterPack(vm.registered_types());

  Error prepSuccess = PrepInput(parameterPack, params, vm, exec.get(), func,
                                "Exec: " + execName + " State: " + stateName);
  if (prepSuccess.code() != Error::Code::SUCCESS)
  {
    return ExecutionResult(LedgerVariant{}, prepSuccess, "");
  }

  // Run
  std::string runTimeError;
  VmVariant   vmOutput;

  bool allOK = vm.Execute(*exec, entrypoint, runTimeError, vmOutput, parameterPack);
  if (!allOK || !runTimeError.empty())
  {
    return ExecutionResult{
        LedgerVariant{},
        Error{Error::Stage::RUNNING, Error::Code::RUNTIME_ERROR, std::move(runTimeError)},
        console.str()};
  }

  return PrepOutput(vm, exec.get(), vmOutput, console.str(),
                    "Exec:" + execName + " with state " + stateName);
}

ExecutionResult BasicVmEngine::EngineError(Error::Code code, std::string errorMessage) const
{
  return ExecutionResult{LedgerVariant(),
                         Error{Error::Stage::ENGINE, code, std::move(errorMessage)}, std::string{}};
}

ExecutionResult BasicVmEngine::EngineSuccess(std::string successMessage) const
{
  return ExecutionResult{
      LedgerVariant(), Error{Error::Stage::ENGINE, Error::Code::SUCCESS, std::move(successMessage)},
      std::string{}};
}

bool BasicVmEngine::HasExecutable(std::string const &name) const
{
  return executables_.find(name) != executables_.end();
}

bool BasicVmEngine::HasState(std::string const &name) const
{
  return states_.find(name) != states_.end();
}

BasicVmEngine::Error BasicVmEngine::PrepInput(vm::ParameterPack &result,
                                              Params const &params, VM &vm, Executable *exec,
                                              Executable::Function const *func,
                                              std::string const &         runName)
{
  const std::string errorPrefix = "Error( " + runName + "): ";

  auto const numParameters = static_cast<std::size_t>(func->num_parameters);
  if (numParameters != params.size())
  {
    return Error(Error::Stage::ENGINE, Error::Code::RUNTIME_ERROR,
                 errorPrefix + "Wrong number of parameters expected " +
                     std::to_string(numParameters) + "; received " + std::to_string(params.size()));
  }

  // Serialize LedgerVariant to MsgPack...
  serializers::MsgPackSerializer serializer;
  try
  {
    for (auto const &p : params)
    {
      serializer << p;
    }
  }
  catch (std::exception &ex)
  {
    return Error(
        Error::Stage::ENGINE, Error::Code::SERIALIZATION_ERROR,
        errorPrefix + "Serializing input before running: Threw error " + std::string(ex.what()));
  }
  catch (...)
  {
    return Error(Error::Stage::ENGINE, Error::Code::SERIALIZATION_ERROR,
                 errorPrefix + "Serializing input before running:  No details");
  }
  serializer.seek(0);

  // ... then deserialize to VmVariant
  ExecutionContext executionContext(&vm, exec);
  for (std::size_t i = 0; i < numParameters; ++i)
  {
    auto type_id = func->variables[i].type_id;
    if (type_id <= vm::TypeIds::PrimitiveMaxId)
    {
      VmVariant param;

      serializer >> param.primitive.i64;
      param.type_id = type_id;

      result.AddSingle(param);
    }
    else
    {
      // Checking if we can construct the object
      if (!vm.IsDefaultSerializeConstructable(type_id))
      {
        return Error{Error::Stage::ENGINE, Error::Code::RUNTIME_ERROR,
                     errorPrefix + "Parameter " + std::to_string(i) + " Could not construct type " +
                         vm.GetTypeName(type_id)};
      }

      // Creating the object
      vm::Ptr<vm::Object> object  = vm.DefaultSerializeConstruct(type_id);
      auto                success = object->DeserializeFrom(serializer);

      // If deserialization failed we return
      if (!success)
      {
        return Error{Error::Stage::ENGINE, Error::Code::RUNTIME_ERROR,
                     errorPrefix + "Parameter " + std::to_string(i) +
                         " Could not deserialize type " + vm.GetTypeName(type_id)};
      }

      // Adding the parameter to the parameter pack
      result.AddSingle(object);
    }
  }
  return Error{Error::Stage::ENGINE, Error::Code::SUCCESS, ""};
}

ExecutionResult BasicVmEngine::PrepOutput(VM &vm, Executable *exec, VmVariant const &vmVariant,
                                          std::string const &console, std::string &&id) const
{
  auto serializationError = [&](std::string &&errorMessage) {
    return ExecutionResult{LedgerVariant{},
                           Error{Error::Stage::ENGINE, Error::Code::SERIALIZATION_ERROR,
                                 "Error(" + id + ") in output after running. " + errorMessage},
                           console};
  };

  LedgerVariant output;
  if (vmVariant.type_id <= vm::TypeIds::PrimitiveMaxId)
  {
    switch (vmVariant.type_id)
    {
    case fetch::vm::TypeIds::Bool:
    {
      output = vmVariant.Get<bool>();
      break;
    }
    case fetch::vm::TypeIds::Int8:
    case fetch::vm::TypeIds::UInt8:
    case fetch::vm::TypeIds::Int16:
    case fetch::vm::TypeIds::UInt16:
    case fetch::vm::TypeIds::Int32:
    case fetch::vm::TypeIds::UInt32:
    case fetch::vm::TypeIds::Int64:
    {
      output = vmVariant.Get<int>();
      break;
    }
    case fetch::vm::TypeIds::Float32:
    {
      output = vmVariant.Get<float>();
      break;
    }
    case fetch::vm::TypeIds::Float64:
    {
      output = vmVariant.Get<double>();
      break;
    }
    case fetch::vm::TypeIds::Fixed32:
    {
      output = vmVariant.Get<fp32_t>();
      break;
    }
    case fetch::vm::TypeIds::Fixed64:
    {
      output = vmVariant.Get<fp64_t>();
      break;
    }
    case vm::TypeIds::Void:
    case vm::TypeIds::Unknown:
    {
      break;
    }
    default:
      return serializationError("Could not transform primitive type " +
                                vm.GetTypeName(vmVariant.type_id));
    }
  }
  else if (vmVariant.type_id == vm::TypeIds::String)
  {
    output = vmVariant.Get<vm::Ptr<vm::String>>()->str;
  }
  else
  {
    ExecutionContext executionContext(&vm, exec);

    auto inside = vmVariant.Get<vm::Ptr<vm::Object>>();

    serializers::MsgPackSerializer serializer;
    try
    {
      inside->SerializeTo(serializer);
      serializer.seek(0);
    }
    catch (std::exception &ex)
    {
      return serializationError("Serializing output threw error " + std::string(ex.what()));
    }
    catch (...)
    {
      return serializationError("Serializing output threw error. No details.");
    }

    try
    {
      serializer >> output;
    }
    catch (std::exception &ex)
    {
      return serializationError(" Deserializing output after running. Threw error " +
                                std::string(ex.what()));
    }
    catch (...)
    {
      return serializationError(" Deserializing output after running. No details.");
    }

    if (output.IsArray())  // Convert inner type from int to fixedpoint if necessary
    {
      auto GetInnermostTypeId = [&vm](LedgerVariant const &ledgerVariant,
                                      VmVariant     const &vmVariant) {
        int  depth   = 0;
        LedgerVariant const *current = &ledgerVariant;
        while (current->IsArray())
        {
          ++depth;
          current = &(*current)[0];
        }

        auto result = vmVariant.type_id;
        if (depth >= 1)
        {
          auto currentTypeId = vmVariant.type_id;
          for (int i = 0; i < depth - 1; ++i)
          {
            currentTypeId = vm.GetTypeInfo(currentTypeId).template_parameter_type_ids[0];
          }
          result = vm.GetTypeInfo(currentTypeId).template_parameter_type_ids[0];
        }

        return result;
      };

      auto const innermostType = GetInnermostTypeId(output, vmVariant);

      if (innermostType == vm::TypeIds::Fixed64)
      {
        std::function<void(LedgerVariant &)> toFixed64;
        toFixed64 = [&toFixed64](LedgerVariant &var) {
          if (var.IsArray())
          {
            for (std::size_t i = 0; i < var.size(); ++i)
            {
              toFixed64(var[i]);
            }
          }
          else
          {
            var = LedgerVariant(fp64_t::FromBase(var.As<int64_t>()));
          }
        };
        toFixed64(output);
      }
      else if (innermostType == vm::TypeIds::Fixed32)
      {
        std::function<void(LedgerVariant &)> toFixed32;
        toFixed32 = [&toFixed32](LedgerVariant &var) {
          if (var.IsArray())
          {
            for (std::size_t i = 0; i < var.size(); ++i)
            {
              toFixed32(var[i]);
            }
          }
          else
          {
            fp32_t val32;
            val32.Data() = var.As<int>();
            fp64_t val64{fp64_t::FromBase(static_cast<int64_t>(val32.Data()) << 16)};
            var           = LedgerVariant{val64};
          }
        };
        toFixed32(output);
      }
    }
  }
  return ExecutionResult{
      output, Error{Error::Stage::RUNNING, Error::Code::SUCCESS, "Ran " + std::move(id)}, console};
}

BasicVmEngine::ExecutionContext::ExecutionContext(VM *vm, Executable *executable)
  : vm_(vm)
{
  vm_->LoadExecutable(executable);
}
BasicVmEngine::ExecutionContext::~ExecutionContext()
{
  vm_->UnloadExecutable();
}

}  // namespace dmlf
}  // namespace fetch
