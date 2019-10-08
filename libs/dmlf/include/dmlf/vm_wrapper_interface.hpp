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

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace dmlf {

class VmWrapperInterface
{
public:
  VmWrapperInterface()          = default;
  virtual ~VmWrapperInterface() = default;

  using OutputHandler = std::function<void(const std::string &)>;
  using InputHandler  = std::function<std::string(void)>;
  using Params        = std::vector<std::string>;
  using Flags         = std::vector<std::string>;
  using Status        = enum {
    UNCONFIGURED,
    WAITING,
    COMPILING,
    COMPILED,
    RUNNING,
    COMPLETED,
    FAILED_COMPILATION,
    FAILED_RUN,
  };

  virtual std::vector<std::string> Setup(const Flags &flags)                                    = 0;
  virtual std::vector<std::string> Load(std::string source)                                     = 0;
  virtual void                     Execute(const std::string &entrypoint, const Params &params) = 0;
  virtual void                     SetStdout(OutputHandler)                                     = 0;
  virtual void                     SetStderr(OutputHandler)                                     = 0;
  virtual Status                   status() const                                               = 0;

  VmWrapperInterface(const VmWrapperInterface &other) = delete;
  VmWrapperInterface &operator=(const VmWrapperInterface &other)  = delete;
  bool                operator==(const VmWrapperInterface &other) = delete;
  bool                operator<(const VmWrapperInterface &other)  = delete;

protected:
private:
};

}  // namespace dmlf
}  // namespace fetch
