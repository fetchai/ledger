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

class VmWrapperSystemcommand: public VmWrapperInterface
{
public:
  using OutputHandler = VmWrapperInterface::OutputHandler;
  using InputHandler  = VmWrapperInterface::InputHandler;
  using Params        = VmWrapperInterface::Params;
  using Flags         = VmWrapperInterface::Flags;
  using Status        = VmWrapperInterface::Status;

  VmWrapperSystemcommand()
  {
  }
  virtual ~VmWrapperSystemcommand()
  {
  }

  std::vector<std::string> Setup(const Flags &flags)
  {
  }
  std::vector<std::string> Load(std::string source)
  {
    status = VmWrapperInterface::COMPILING;
    commmand_ = source;
    status = VmWrapperInterface::COMPILED;
    return std::vector<std::string>();
  }
  virtual void Execute(std::string entrypoint, const Params params)
  {
  }
  virtual void SetStdout(OutputHandler) = 0;
  virtual void SetStdin(InputHandler) = 0;
  virtual void SetStderr(OutputHandler) = 0;

  virtual Status status(void) const { return status_; }
protected:
  Status status_ = VmWrapperInterface::WAITING;

  enum {
    WRAPPER_SIDE = 1,
    COMMAND_SIDE = 0,
  };
  int stdin_pipe[2];
  int stderr_pipe[2];
  int stdout_pipe[2];
private:
  VmWrapperSystemcommand(const VmWrapperSystemcommand &other) = delete;
  VmWrapperSystemcommand &operator=(const VmWrapperSystemcommand &other) = delete;
  bool operator==(const VmWrapperSystemcommand &other) = delete;
  bool operator<(const VmWrapperSystemcommand &other) = delete;
};
