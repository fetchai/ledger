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

#include "mock_io_observer.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include "gmock/gmock.h"

#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromHex;
using fetch::vm::ChargeAmount;
using fetch::vm::Compiler;
using fetch::vm::Executable;
using fetch::vm::IR;
using fetch::vm::Module;
using fetch::vm::Variant;
using fetch::vm::VM;
using fetch::vm_modules::VMFactory;
using testing::_;

using ExecutablePtr = std::unique_ptr<Executable>;
using CompilerPtr   = std::unique_ptr<Compiler>;
using ModulePtr     = std::shared_ptr<Module>;
using IRPtr         = std::unique_ptr<IR>;
using VMPtr         = std::unique_ptr<VM>;
using ObserverPtr   = std::unique_ptr<MockIoObserver>;

class VmTestToolkit
{
public:
  explicit VmTestToolkit(std::ostream *stdout = nullptr)
    : stdout_{stdout ? stdout : &std::cout}
    , observer_{std::make_unique<MockIoObserver>()}
    , module_{fetch::vm_modules::VMFactory::GetModule(VMFactory::USE_ALL)}
  {}

  bool Compile(std::string const &text)
  {
    std::vector<std::string> errors{};

    // build the compiler and IR
    compiler_ = std::make_unique<Compiler>(module_.get());
    ir_       = std::make_unique<IR>();

    // compile the source code
    fetch::vm::SourceFiles files = {{"default.etch", text}};
    if (!compiler_->Compile(files, "default_ir", *ir_, errors))
    {
      PrintErrors(errors);
      return false;
    }

    // build the executable
    executable_ = std::make_unique<Executable>();
    vm_         = std::make_unique<VM>(module_.get());
    vm_->SetIOObserver(*observer_);
    vm_->AttachOutputDevice(fetch::vm::VM::STDOUT, *stdout_);

    if (!vm_->GenerateExecutable(*ir_, "default_exe", *executable_, errors))
    {
      PrintErrors(errors);
      return false;
    }

    return true;
  }

  bool Run(Variant *    output       = nullptr,
           ChargeAmount charge_limit = std::numeric_limits<ChargeAmount>::max())
  {
    return RunWithParams(output, charge_limit);
  }

  template <typename... Ts>
  bool RunWithParams(Variant *output, ChargeAmount charge_limit, Ts &&... parameters)
  {
    vm_->SetChargeLimit(charge_limit);
    std::string error{};

    Variant dummy_output{};
    if (output == nullptr)
    {
      output = &dummy_output;
    }

    if (!vm_->Execute(*executable_, "main", error, *output, std::forward<Ts...>(parameters)...))
    {
      *stdout_ << "Runtime Error: " << error << std::endl;

      return false;
    }

    return true;
  }

  void PrintErrors(VMFactory::Errors const &errors)
  {
    for (auto const &line : errors)
    {
      *stdout_ << "Compiler Error: " << line << std::endl;
    }
    *stdout_ << std::endl;
  }

  void AddState(std::string const &key, ConstByteArray const &hex_value)
  {
    auto raw_value = FromHex(hex_value);

    observer_->fake_.SetKeyValue(key, raw_value);
  }

  Module &module() const
  {
    return *module_;
  }

  VM &vm() const
  {
    return *vm_;
  }

  MockIoObserver &observer() const
  {
    return *observer_;
  }

  void setStdout(std::ostream &ostream)
  {
    stdout_ = &ostream;
  }

private:
  std::ostream *stdout_ = &std::cout;
  ObserverPtr   observer_;
  ModulePtr     module_;
  CompilerPtr   compiler_;
  IRPtr         ir_;
  ExecutablePtr executable_;
  VMPtr         vm_;
};
