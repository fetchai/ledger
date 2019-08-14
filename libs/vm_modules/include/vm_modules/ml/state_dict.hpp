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

#include "core/serializers/main_serializer.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/state_dict.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/type.hpp"

#include <utility>

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMStateDict : public fetch::vm::Object
{
public:
  using DataType = fetch::vm_modules::math::DataType;

  VMStateDict(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  VMStateDict(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
              fetch::ml::StateDict<fetch::math::Tensor<fetch::vm_modules::math::DataType>> sd);

  static fetch::vm::Ptr<VMStateDict> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  void SetWeights(fetch::vm::Ptr<fetch::vm::String> const &                nodename,
                  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &weights);

  static void Bind(fetch::vm::Module &module);

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  fetch::ml::StateDict<fetch::math::Tensor<DataType>> state_dict_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
