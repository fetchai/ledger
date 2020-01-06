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

#include "core/serializers/main_serializer.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vm/array.hpp"
#include "vm/object.hpp"
#include "vm/pair.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"

#include <memory>

namespace fetch {
namespace vm_modules {
namespace math {
class VMTensor;
}
namespace ml {

class VMDataLoader : public fetch::vm::Object
{
public:
  using DataType          = fetch::vm_modules::math::DataType;
  using TensorType        = fetch::math::Tensor<DataType>;
  using DataLoaderType    = fetch::ml::dataloaders::DataLoader<TensorType, TensorType>;
  using DataLoaderPtrType = std::shared_ptr<DataLoaderType>;

  enum class DataLoaderMode : uint8_t
  {
    NONE,
    TENSOR,
  };

  VMDataLoader(fetch::vm::VM *vm, fetch::vm::TypeId type_id);
  VMDataLoader(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
               fetch::vm::Ptr<fetch::vm::String> const &mode);

  static fetch::vm::Ptr<VMDataLoader> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                  fetch::vm::Ptr<fetch::vm::String> const &mode);

  static void Bind(fetch::vm::Module &module, bool enable_experimental);

  /**
   * Add data to a data loader by passing in the data and labels
   * @param mode
   * @param data
   * @param labels
   */
  void AddDataByData(
      fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>> const
          &                                                    data,
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &labels);

  /**
   * Add data to tensor data loader
   * @param xfilename
   * @param yfilename
   */
  void AddTensorData(
      fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>> const
          &                                                    data,
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &labels);

  /**
   * Get the next training pair of data and labels from the dataloader
   * @return
   */
  vm::Ptr<vm::Pair<vm::Ptr<math::VMTensor>,
                   vm::Ptr<fetch::vm::Array<vm::Ptr<fetch::vm_modules::math::VMTensor>>>>>
  GetNext();

  bool IsDone();

  DataLoaderPtrType &GetDataLoader();

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

private:
  DataLoaderPtrType loader_;
  DataLoaderMode    mode_ = DataLoaderMode::NONE;
};

}  // namespace ml
}  // namespace vm_modules

namespace serializers {

/**
 * serializer for tensor dataloader
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<fetch::vm_modules::ml::VMDataLoader, D>
{
  using Type       = fetch::vm_modules::ml::VMDataLoader;
  using DriverType = D;

  static uint8_t const MODE       = 1;
  static uint8_t const HAS_LOADER = 2;
  static uint8_t const LOADER     = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    map.Append(MODE, static_cast<uint8_t>(sp.mode_));

    if (sp.loader_)
    {
      map.Append(HAS_LOADER, true);

      switch (sp.mode_)
      {
      case vm_modules::ml::VMDataLoader::DataLoaderMode::TENSOR:
      {
        auto tdl_ptr = std::static_pointer_cast<fetch::ml::dataloaders::TensorDataLoader<
            fetch::math::Tensor<vm_modules::math::DataType>,
            fetch::math::Tensor<vm_modules::math::DataType>>>(sp.loader_);
        map.Append(LOADER, *tdl_ptr);
        break;
      }
      default:
      {
        // this dataloader has no mode set, so it should not be serialisable
        throw std::runtime_error("no mode specified for dataloader - serialisation not permitted");
      }
      }
    }
    else
    {
      map.Append(HAS_LOADER, false);
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    uint8_t mode{};
    map.ExpectKeyGetValue(MODE, mode);
    sp.mode_ = static_cast<vm_modules::ml::VMDataLoader::DataLoaderMode>(mode);

    bool has_loader{};
    map.ExpectKeyGetValue(HAS_LOADER, has_loader);
    if (has_loader)
    {
      switch (sp.mode_)
      {
      case vm_modules::ml::VMDataLoader::DataLoaderMode::TENSOR:
      {
        auto tdl_ptr = std::make_shared<fetch::ml::dataloaders::TensorDataLoader<
            fetch::math::Tensor<vm_modules::math::DataType>,
            fetch::math::Tensor<vm_modules::math::DataType>>>();
        map.ExpectKeyGetValue(LOADER, *tdl_ptr);

        sp.loader_ = std::static_pointer_cast<
            fetch::ml::dataloaders::DataLoader<fetch::math::Tensor<vm_modules::math::DataType>,
                                               fetch::math::Tensor<vm_modules::math::DataType>>>(
            tdl_ptr);

        break;
      }
      default:
      {
        // this dataloader didn't have any data added, so doesn't have a
        throw std::runtime_error("cannot deserialise dataloader with no mode specified");
      }
      }
    }
  }
};

}  // namespace serializers
}  // namespace fetch
