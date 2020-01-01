#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/utilities/min_max_scaler.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/type.hpp"

#include <memory>

namespace fetch {

namespace ml {
namespace utilities {
template <typename>
class Scaler;
}
}  // namespace ml
namespace vm {
class Module;
}

namespace vm_modules {

namespace math {
class VMTensor;
}

namespace ml {
namespace utilities {

class VMScaler : public fetch::vm::Object
{
public:
  using DataType   = fetch::vm_modules::math::DataType;
  using ScalerType = fetch::ml::utilities::Scaler<fetch::math::Tensor<DataType>>;

  VMScaler(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMScaler> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  void SetScaleByData(fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &reference_tensor,
                      fetch::vm::Ptr<fetch::vm::String> const &                mode);

  void SetScaleByRange(DataType const &min_val, DataType const &max_val);

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> Normalise(
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &input_tensor);

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> DeNormalise(
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &input_tensor);

  static void Bind(fetch::vm::Module &module, bool enable_experimental);

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  std::shared_ptr<ScalerType> scaler_;
};

}  // namespace utilities
}  // namespace ml
}  // namespace vm_modules

namespace serializers {

/**
 * serializer for VMScaler
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<fetch::vm_modules::ml::utilities::VMScaler, D>
{
  using Type       = fetch::vm_modules::ml::utilities::VMScaler;
  using DriverType = D;

  static uint8_t const SCALER = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(SCALER, (*std::static_pointer_cast<fetch::ml::utilities::MinMaxScaler<
                            fetch::math::Tensor<fetch::vm_modules::math::DataType>>>(sp.scaler_)));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto scaler_ptr = std::make_shared<fetch::ml::utilities::MinMaxScaler<
        fetch::math::Tensor<fetch::vm_modules::math::DataType>>>();
    map.ExpectKeyGetValue(SCALER, *(scaler_ptr));
    sp.scaler_ = std::static_pointer_cast<Type::ScalerType>(scaler_ptr);
  }
};

}  // namespace serializers
}  // namespace fetch
