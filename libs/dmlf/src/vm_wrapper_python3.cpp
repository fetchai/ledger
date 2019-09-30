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

#include "dmlf/vm_wrapper_python3.hpp"

//#include <python.h>

namespace fetch {
namespace dmlf {

typedef int PyEnvName;

class PyEnvInner
{
public:
  static int py_env_counter;

  PyEnvInner()
  {
    if (py_env_counter == 0)
    {
      //::Py_Initialize();
    }
    py_env_counter++;
  }

  virtual ~PyEnvInner()
  {
    py_env_counter--;
    if (py_env_counter == 0)
    {
      //::Py_FinalizeEx();
    }
  }
};

int PyEnvInner::py_env_counter = 0;

VmWrapperPython3::VmWrapperPython3()
{
  py_ = std::make_shared<PyEnvInner>();
}
VmWrapperPython3::~VmWrapperPython3()
{
  py_.reset();
}
}  // namespace dmlf
}  // namespace fetch
