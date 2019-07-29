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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/module.hpp"
#include "vm_modules/core/type_convert.hpp"

#include <cstdint>
#include <sstream>
#include <type_traits>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

namespace {

/**
 * method for type converting from arithmetic to string
 */
template <typename T>
fetch::math::meta::IfIsNonFixedPointArithmetic<T, Ptr<String>> ToString(VM *vm, T const &a)
{
  if (std::is_same<T, bool>::value)
  {
    Ptr<String> ret(new String(vm, static_cast<bool>(a) ? "true" : "false"));
    return ret;
  }
  else
  {
    Ptr<String> ret(new String(vm, std::to_string(a)));
    return ret;
  }
}

template <typename T>
fetch::math::meta::IfIsFixedPoint<T, Ptr<String>> ToString(VM *vm, T const &a)
{
  std::stringstream stream;
  stream << a;
  Ptr<String> ret(new String(vm, stream.str()));
  return ret;
}

template <typename T>
bool ToBool(VM * /*vm*/, T const &a)
{
  return static_cast<bool>(a);
}

}  // namespace

void CreateToString(Module &module)
{
  module.CreateFreeFunction("toString", &ToString<int8_t>);
  module.CreateFreeFunction("toString", &ToString<uint8_t>);
  module.CreateFreeFunction("toString", &ToString<int16_t>);
  module.CreateFreeFunction("toString", &ToString<uint16_t>);
  module.CreateFreeFunction("toString", &ToString<int32_t>);
  module.CreateFreeFunction("toString", &ToString<uint32_t>);
  module.CreateFreeFunction("toString", &ToString<int64_t>);
  module.CreateFreeFunction("toString", &ToString<uint64_t>);
  module.CreateFreeFunction("toString", &ToString<float_t>);
  module.CreateFreeFunction("toString", &ToString<double_t>);
  module.CreateFreeFunction("toString", &ToString<bool>);
  module.CreateFreeFunction("toString", &ToString<fixed_point::fp32_t>);
  module.CreateFreeFunction("toString", &ToString<fixed_point::fp64_t>);
}

void CreateToBool(Module &module)
{

  module.CreateFreeFunction("toBool", &ToBool<int32_t>);
  module.CreateFreeFunction("toBool", &ToBool<uint32_t>);
  module.CreateFreeFunction("toBool", &ToBool<int64_t>);
  module.CreateFreeFunction("toBool", &ToBool<uint64_t>);
  module.CreateFreeFunction("toBool", &ToBool<float_t>);
  module.CreateFreeFunction("toBool", &ToBool<double_t>);
}

}  // namespace vm_modules
}  // namespace fetch
