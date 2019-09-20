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

#include "dmlf/vm_wrapper_interface.hpp"

//#include "vm/generator.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <memory>

namespace fetch {
namespace dmlf {

class vm_wrapper_etch: public VmWrapperInterface
{
public:
  using OutputHandler = VmWrapperInterface::OutputHandler;
  using InputHandler  = VmWrapperInterface::InputHandler;
  using Params        = VmWrapperInterface::Params;
  using Flags         = VmWrapperInterface::Flags;
  using Status        = VmWrapperInterface::Status;

  using VmFactory = fetch::vm_modules::VMFactory;
  using VM = fetch::vm::VM;
  using Executable = fetch::vm::Executable;

  vm_wrapper_etch()
  {
  }
  virtual ~vm_wrapper_etch()
  {
  }

  virtual std::vector<std::string> Setup(const Flags &flags);
  virtual std::vector<std::string> Load(std::string source);
  virtual void Execute(std::string entrypoint, const Params params);
  virtual void SetStdout(OutputHandler) = 0;
  virtual void SetStdin(InputHandler) = 0;
  virtual void SetStderr(OutputHandler) = 0;

  virtual Status status(void) const { return status_; }
protected:
private:
  Status status_ = VmWrapperInterface::UNCONFIGURED;
  
  std::string command_ = "";
  std::unique_ptr<Executable> executable_ = std::make_unique<Executable>();
  std::shared_ptr<fetch::vm::Module> module_ = nullptr;
  std::unique_ptr<VM> vm_ = nullptr;

  vm_wrapper_etch(const vm_wrapper_etch &other) = delete;
  vm_wrapper_etch &operator=(const vm_wrapper_etch &other) = delete;
  bool operator==(const vm_wrapper_etch &other) = delete;
  bool operator<(const vm_wrapper_etch &other) = delete;
};

} // namespace dmlf
} // namespace fetch
