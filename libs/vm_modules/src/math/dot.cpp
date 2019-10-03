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

#include "math/meta/math_type_traits.hpp"
#include "math/exceptions/exceptions.hpp"
#include "math/matrix_operations.hpp"
#include "vm/module.hpp"
#include "vm_modules/math/dot.hpp"
#include "vm_modules/math/tensor.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

namespace {

VMTensor Dot(VM * /*vm*/, VMTensor &a, VMTensor &b)
{
  typename VMTensor::TensorType x;
  try
  {
    fetch::math::Dot(a.GetTensor(), b.GetTensor(), x);
  }
  catch (const fetch::math::exceptions::WrongShape &e)
  {
    std::cerr << e.what();
  }

  return VMTensor(a.vm_, a.type_id_, ret);
}

}  // namespace


void BindDot(Module &module)
{
  module.CreateFreeFunction<VMTensor (*)(VM *, VMTensor &, VMTensor &)>("dot", &Dot);
}


}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
