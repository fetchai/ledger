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

#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vm/array.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/type.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace fetch {

namespace math {
template <typename T, typename C>
class Tensor;
}
namespace ml {
template <class T>
class Graph;
namespace optimisers {

template <class T>
class Optimiser;

template <class T>
class AdaGradOptimiser;

template <class T>
class AdamOptimiser;

template <class T>
class MomentumOptimiser;

template <class T>
class RMSPropOptimiser;

template <class T>
class SGDOptimiser;

}  // namespace optimisers
}  // namespace ml
namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMOptimiser : public fetch::vm::Object
{
public:
  using DataType   = fetch::vm_modules::math::DataType;
  using TensorType = fetch::math::Tensor<VMOptimiser::DataType>;
  using GraphType  = fetch::ml::Graph<TensorType>;

  using OptimiserType         = fetch::ml::optimisers::Optimiser<TensorType>;
  using AdagradOptimiserType  = fetch::ml::optimisers::AdaGradOptimiser<TensorType>;
  using AdamOptimiserType     = fetch::ml::optimisers::AdamOptimiser<TensorType>;
  using MomentumOptimiserType = fetch::ml::optimisers::MomentumOptimiser<TensorType>;
  using RmspropOptimiserType  = fetch::ml::optimisers::RMSPropOptimiser<TensorType>;
  using SgdOptimiserType      = fetch::ml::optimisers::SGDOptimiser<TensorType>;

  enum class OptimiserMode : uint8_t
  {
    NONE,
    ADAGRAD,
    ADAM,
    MOMENTUM,
    RMSPROP,
    SGD
  };

  VMOptimiser(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  VMOptimiser(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &mode,
              fetch::ml::Graph<fetch::math::Tensor<DataType>> const &graph,
              vm::Ptr<VMDataLoader> const &loader, std::vector<std::string> const &input_node_names,
              std::string const &label_node_name, std::string const &output_node_name);

  static void Bind(vm::Module &module, bool enable_experimental);

  static fetch::vm::Ptr<VMOptimiser> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &mode,
      fetch::vm::Ptr<fetch::vm_modules::ml::VMGraph> const &                     graph,
      fetch::vm::Ptr<fetch::vm_modules::ml::VMDataLoader> const &                loader,
      fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm::String>>> const &input_node_names,
      fetch::vm::Ptr<fetch::vm::String> const &                                  label_node_name,
      fetch::vm::Ptr<fetch::vm::String> const &                                  output_node_names);

  DataType RunData(fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &data,
                   fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &labels,
                   uint64_t                                                 batch_size);

  DataType RunLoader(uint64_t batch_size, uint64_t subset_size);

  DataType RunLoaderNoSubset(uint64_t batch_size);

  void SetGraph(vm::Ptr<VMGraph> const &graph);

  void SetDataloader(vm::Ptr<VMDataLoader> const &loader);

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

private:
  std::shared_ptr<fetch::ml::optimisers::Optimiser<fetch::math::Tensor<DataType>>> optimiser_;
  std::shared_ptr<fetch::ml::dataloaders::DataLoader<TensorType, TensorType>>      loader_;
  OptimiserMode                                                                    mode_;
};

}  // namespace ml
}  // namespace vm_modules

namespace serializers {

/**
 * serializer for VMOptimiser
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<fetch::vm_modules::ml::VMOptimiser, D>
{
  using Type                  = fetch::vm_modules::ml::VMOptimiser;
  using OptimiserType         = fetch::vm_modules::ml::VMOptimiser::OptimiserType;
  using AdagradOptimiserType  = fetch::vm_modules::ml::VMOptimiser::AdagradOptimiserType;
  using AdamOptimiserType     = fetch::vm_modules::ml::VMOptimiser::AdamOptimiserType;
  using MomentumOptimiserType = fetch::vm_modules::ml::VMOptimiser::MomentumOptimiserType;
  using RmspropOptimiserType  = fetch::vm_modules::ml::VMOptimiser::RmspropOptimiserType;
  using SgdOptimiserType      = fetch::vm_modules::ml::VMOptimiser::SgdOptimiserType;

  using DataLoaderType =
      fetch::ml::dataloaders::DataLoader<vm_modules::ml::VMOptimiser::TensorType,
                                         vm_modules::ml::VMOptimiser::TensorType>;
  using TensorDataLoaderType =
      fetch::ml::dataloaders::TensorDataLoader<vm_modules::ml::VMOptimiser::TensorType,
                                               vm_modules::ml::VMOptimiser::TensorType>;
  using DriverType = D;

  static uint8_t const MODE       = 1;
  static uint8_t const HAS_LOADER = 2;
  static uint8_t const LOADER     = 3;
  static uint8_t const OPTIMISER  = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    map.Append(MODE, static_cast<uint8_t>(sp.mode_));

    // optimiser only handles tensor dataloaders for serialisation at the moment
    if (sp.loader_)
    {
      map.Append(HAS_LOADER, true);
      map.Append(LOADER, *(std::static_pointer_cast<TensorDataLoaderType>(sp.loader_)));
    }
    else
    {
      map.Append(HAS_LOADER, false);
    }

    switch (sp.mode_)
    {
    case vm_modules::ml::VMOptimiser::OptimiserMode::ADAGRAD:
    {
      throw std::runtime_error("serialisation not yet implemented for adagrad optimiser");
    }
    case vm_modules::ml::VMOptimiser::OptimiserMode::ADAM:
    {
      auto opt_ptr =
          std::static_pointer_cast<fetch::vm_modules::ml::VMOptimiser::AdamOptimiserType>(
              sp.optimiser_);
      map.Append(OPTIMISER, *opt_ptr);
      break;
    }
    case vm_modules::ml::VMOptimiser::OptimiserMode::MOMENTUM:
    {
      throw std::runtime_error("serialisation not yet implemented for momentum optimiser");
    }
    case vm_modules::ml::VMOptimiser::OptimiserMode::RMSPROP:
    {
      throw std::runtime_error("serialisation not yet implemented for rmsprop optimiser");
    }
    case vm_modules::ml::VMOptimiser::OptimiserMode::SGD:
    {
      auto opt_ptr = std::static_pointer_cast<fetch::vm_modules::ml::VMOptimiser::SgdOptimiserType>(
          sp.optimiser_);
      map.Append(OPTIMISER, *opt_ptr);
      break;
    }
    default:
    {
      throw std::runtime_error("unknown optimiser type, serialisation is not possible.");
    }
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    uint8_t mode;
    map.ExpectKeyGetValue(MODE, mode);
    sp.mode_ = static_cast<vm_modules::ml::VMOptimiser::OptimiserMode>(mode);

    bool has_loader;
    map.ExpectKeyGetValue(HAS_LOADER, has_loader);
    if (has_loader)
    {
      auto tdl_ptr = std::make_shared<TensorDataLoaderType>();
      map.ExpectKeyGetValue(LOADER, *(tdl_ptr));
      sp.loader_ = std::static_pointer_cast<DataLoaderType>(tdl_ptr);
    }

    switch (sp.mode_)
    {
    case vm_modules::ml::VMOptimiser::OptimiserMode::ADAGRAD:
    {
      throw std::runtime_error("deserialisation not yet implemented for adagrad optimiser");
    }
    case vm_modules::ml::VMOptimiser::OptimiserMode::ADAM:
    {
      auto opt_ptr = std::make_shared<AdamOptimiserType>();
      map.ExpectKeyGetValue(OPTIMISER, *opt_ptr);
      sp.optimiser_ = std::static_pointer_cast<OptimiserType>(opt_ptr);
      break;
    }
    case vm_modules::ml::VMOptimiser::OptimiserMode::MOMENTUM:
    {
      throw std::runtime_error("deserialisation not yet implemented for momentum optimiser");
    }
    case vm_modules::ml::VMOptimiser::OptimiserMode::RMSPROP:
    {
      throw std::runtime_error("deserialisation not yet implemented for rmsprop optimiser");
    }
    case vm_modules::ml::VMOptimiser::OptimiserMode::SGD:
    {
      auto opt_ptr = std::make_shared<SgdOptimiserType>();
      map.ExpectKeyGetValue(OPTIMISER, *opt_ptr);
      sp.optimiser_ = std::static_pointer_cast<OptimiserType>(opt_ptr);
      break;
    }
    default:
    {
      throw std::runtime_error("optimiser mode not recognised, deserialisation is not possible.");
    }
    }
  }
};

}  // namespace serializers

}  // namespace fetch
