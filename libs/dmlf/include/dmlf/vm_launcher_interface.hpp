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

#include "vm/variant.hpp"

#include <functional>
#include <ostream>
#include <string>
#include <vector>

namespace fetch {
namespace dmlf {

class VmLauncherInterface
{
public:
  VmLauncherInterface() = default;
  virtual ~VmLauncherInterface() = default;

  VmLauncherInterface(const VmLauncherInterface &other) = delete;
  VmLauncherInterface &operator=(const VmLauncherInterface &other)  = delete;

  bool                 operator==(const VmLauncherInterface &other) = delete;
  bool                 operator<(const VmLauncherInterface &other)  = delete;

  using VmOutputHandler = std::ostream;
  using Params          = std::vector<fetch::vm::Variant>;
  // Program name, Error
  using ProgramErrorHandler = std::function<void(std::string const &, std::vector<std::string>)>;
  // Program name, VM name, State name, Error
  using ExecuteErrorHandler = std::function<void(std::string const &, std::string const &,
                                                 std::string const &, std::string const &)>;

  virtual bool CreateProgram(std::string name, std::string const &source) = 0;
  virtual bool HasProgram(std::string const &name) const                  = 0;
  virtual void AttachProgramErrorHandler(ProgramErrorHandler)             = 0;

  virtual bool CreateVM(std::string name)                                = 0;
  virtual bool HasVM(std::string const &name) const                      = 0;
  virtual bool SetVmStdout(std::string const &vmName, VmOutputHandler &) = 0;

  virtual bool CreateState(std::string name)                              = 0;
  virtual bool HasState(std::string const &name) const                    = 0;
  virtual bool CopyState(std::string const &srcName, std::string newName) = 0;

  virtual bool Execute(std::string const &program, std::string const &vm, std::string const &state,
                       std::string const &entrypoint, Params const &params) = 0;
  virtual void AttachExecuteErrorHandler(ExecuteErrorHandler)               = 0;
};

}  // namespace dmlf
}  // namespace fetch
