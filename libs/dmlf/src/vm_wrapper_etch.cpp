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

#include "dmlf/vm_wrapper_etch.hpp"

#include "variant/variant.hpp"

namespace fetch {
namespace dmlf {

std::vector<std::string> vm_wrapper_etch::Setup(const Flags &/*flags*/)
{
  module_ = VmFactory::GetModule(VmFactory::USE_SMART_CONTRACTS); // Set according to flags
  status_ = VmWrapperInterface::WAITING;
  return std::vector<std::string>();
}
std::vector<std::string> vm_wrapper_etch::Load(std::string source)
{
  status_ = VmWrapperInterface::COMPILING;
  command_ = source;
  fetch::vm::SourceFiles files = {{"default.etch", source}};  
  auto errors =  VmFactory::Compile(module_, files, *executable_);

  if (!errors.empty()) {
    status_ = VmWrapperInterface::FAILED_COMPILATION;
    return errors;
  }

  // Create the VM instance
  vm_ = std::make_unique<VM>(module_.get());
  vm_->AttachOutputDevice("deviceName", std::cout); // Change for OutputHandler
  status_ = VmWrapperInterface::COMPILED;
  return errors;
}
void vm_wrapper_etch::Execute(std::string entrypoint, const Params /*params*/)
{
      status_ = VmWrapperInterface::RUNNING;
      std::string error;
      std::string console;
      fetch::vm::Variant     output;
      /*auto result = */ vm_->Execute(*executable_, entrypoint, error, output);
      status_ = VmWrapperInterface::COMPLETED;
}

}  // namespace dmlf
}  // namespace fetch
