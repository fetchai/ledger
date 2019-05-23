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

#include "mock_io_observer.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include "gmock/gmock.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

using testing::_;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromHex;
using fetch::vm::VM;
using fetch::vm::IR;
using fetch::vm::Executable;
using fetch::vm::Compiler;
using fetch::vm::Module;
using fetch::vm::Variant;

using VmErrors      = std::vector<std::string>;
using ExecutablePtr = std::unique_ptr<Executable>;
using CompilerPtr   = std::unique_ptr<Compiler>;
using ModulePtr     = std::shared_ptr<Module>;
using IRPtr         = std::unique_ptr<IR>;
using VMPtr         = std::unique_ptr<VM>;
using ObserverPtr   = std::unique_ptr<MockIoObserver>;

class VmTestToolkit
{
public:
  VmTestToolkit()
    : observer_{std::make_unique<MockIoObserver>()}
    , module_{fetch::vm_modules::VMFactory::GetModule()}
  {}

  bool Compile(char const *text)
  {
    std::vector<std::string> errors{};

    // build the compiler and IR
    compiler_ = std::make_unique<Compiler>(module_.get());
    ir_       = std::make_unique<IR>();

    // compile the source code
    if (!compiler_->Compile(text, "default", *ir_, errors))
    {
      PrintErrors(errors);
      return false;
    }

    // build the executable
    executable_ = std::make_unique<Executable>();
    vm_         = std::make_unique<VM>(module_.get());
    vm_->SetIOObserver(*observer_);
    vm_->AttachOutputDevice(fetch::vm::VM::STDOUT, stdout_);

    // generate the IR
    if (!vm_->GenerateExecutable(*ir_, "default_ir", *executable_, errors))
    {
      PrintErrors(errors);
      return false;
    }

    return true;
  }

  bool Run()
  {
    std::string error{};
    Variant     output{};
    if (!vm_->Execute(*executable_, "main", error, output))
    {
      stdout_ << "Runtime Error: " << error << std::endl;

      return false;
    }

    return true;
  }

  void PrintErrors(VmErrors const &errors)
  {
    for (auto const &line : errors)
    {
      stdout_ << "Compiler Error: " << line << '\n';
    }
    stdout_ << std::endl;
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

  MockIoObserver &observer() const
  {
    return *observer_;
  }

private:
  ObserverPtr        observer_;
  ModulePtr          module_;
  CompilerPtr        compiler_;
  IRPtr              ir_;
  ExecutablePtr      executable_;
  VMPtr              vm_;
  std::ostringstream stdout_;
};
