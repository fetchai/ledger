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

#include "vm/module.hpp"
#include "vm_modules/dmlf/update.hpp"
#include "vm_modules/dmlf/colearner.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace dmlf {

void BindDMLF(Module &module)
{
  // Update
  VMUpdate::Bind(module);
  
  // Collaborative Learner
  VMCoLearner::Bind(module);
}

}  // namespace dmlf
}  // namespace vm_modules
}  // namespace fetch

