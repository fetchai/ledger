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

namespace fetch {
namespace dmlf {


class vm_wrapper_python3: public VmWrapperInterface
{
public:
  using OutputHandler = VmWrapperInterface::OutputHandler;
  using InputHandler  = VmWrapperInterface::InputHandler;
  using Params        = VmWrapperInterface::Params;
  using Flags         = VmWrapperInterface::Flags;
  using Status        = VmWrapperInterface::Status;

  vm_wrapper_python3()
  {
  }
  virtual ~vm_wrapper_python3()
  {
  }
  virtual void SetStdout(OutputHandler oh)   { oh_ = oh; }
  virtual void SetStdin(InputHandler ih)     { ih_ = ih; }
  virtual void SetStderr(OutputHandler eh)   { eh_ = eh; }
protected:
  OutputHandler oh_;
  Status status_ = VmWrapperInterface::WAITING;
  OutputHandler eh_;
  InputHandler ih_;
private:
  vm_wrapper_python3(const vm_wrapper_python3 &other) = delete;
  vm_wrapper_python3 &operator=(const vm_wrapper_python3 &other) = delete;
  bool operator==(const vm_wrapper_python3 &other) = delete;
  bool operator<(const vm_wrapper_python3 &other) = delete;
};

}
}
