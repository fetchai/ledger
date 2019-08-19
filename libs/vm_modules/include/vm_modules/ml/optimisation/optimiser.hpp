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
}
}  // namespace ml
namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMOptimiser : public fetch::vm::Object
{
public:

  using DataType = fetch::vm_modules::math::DataType;
  using TensorType = fetch::math::Tensor<VMOptimiser::DataType>;
  using GraphType = fetch::ml::Graph<TensorType>;

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


  VMOptimiser(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &mode,
              fetch::ml::Graph<fetch::math::Tensor<DataType>> const &graph,
              std::vector<std::string> const &input_node_names, std::string const &label_node_name,
              std::string const &output_node_name);

  static void Bind(vm::Module &module);

  static fetch::vm::Ptr<VMOptimiser> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &mode,
      fetch::vm::Ptr<fetch::vm_modules::ml::VMGraph> const &graph,
      fetch::vm::Ptr<fetch::vm::String> const &             input_node_names,
      fetch::vm::Ptr<fetch::vm::String> const &             label_node_name,
      fetch::vm::Ptr<fetch::vm::String> const &             output_node_names);

  DataType RunData(fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &data,
                   fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &labels,
                   uint64_t                                                 batch_size);

  DataType RunLoader(fetch::vm::Ptr<fetch::vm_modules::ml::VMDataLoader> const &loader,
                     uint64_t batch_size, uint64_t subset_size);

  DataType RunLoaderNoSubset(fetch::vm::Ptr<fetch::vm_modules::ml::VMDataLoader> const &loader,
                             uint64_t                                                   batch_size);


  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

private:
  std::shared_ptr<fetch::ml::optimisers::Optimiser<fetch::math::Tensor<DataType>>> optimiser_;
  OptimiserMode mode_;
};

}  // namespace ml
}  // namespace vm_modules


namespace serializers {

/**
 * serializer for tensor dataloader
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<fetch::vm_modules::ml::VMOptimiser, D>
{
  using Type       = fetch::vm_modules::ml::VMOptimiser;
  using DriverType = D;

  static uint8_t const MODE      = 1;
  static uint8_t const OPTIMISER = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    map.Append(MODE, static_cast<uint8_t>(sp.mode_));

    switch (sp.mode_)
    {
      case vm_modules::ml::VMOptimiser::OptimiserMode::ADAGRAD:
      {
        throw std::runtime_error("serialisation not yet implemented for adagrad optimiser");
      }
      case vm_modules::ml::VMOptimiser::OptimiserMode::ADAM:
      {
        throw std::runtime_error("serialisation not yet implemented for adam optimiser");
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
        auto tdl_ptr = std::static_pointer_cast<fetch::vm_modules::ml::VMOptimiser::SgdOptimiserType>(sp.optimiser_);
        map.Append(OPTIMISER, *tdl_ptr);
        break;
      }
    default:
    {
      throw std::runtime_error("unknown dataloader type");
    }
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    uint8_t mode;
    map.ExpectKeyGetValue(MODE, mode);
    sp.mode_ = static_cast<vm_modules::ml::VMOptimiser::OptimiserMode>(mode);

    switch (sp.mode_)
    {
      case vm_modules::ml::VMOptimiser::OptimiserMode::ADAGRAD:
      {
        throw std::runtime_error("serialisation not yet implemented for adagrad optimiser");
      }
      case vm_modules::ml::VMOptimiser::OptimiserMode::ADAM:
      {
        throw std::runtime_error("serialisation not yet implemented for adam optimiser");
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
        optimiser
        throw std::runtime_error("serialisation not yet implemented for sgd optimiser");
      }
      default:
      {
        throw std::runtime_error("optimiser mode not recognised");
      }
    }

    switch (sp.mode_)
    {
    case vm_modules::ml::VMOptimiser::DataLoaderMode::COMMODITY:
    {
      throw std::runtime_error("commodity dataloader deserialization not yet implemented");
    }
    case vm_modules::ml::VMOptimiser::DataLoaderMode::MNIST:
    {
      throw std::runtime_error("mnist dataloader deserialization not yet implemented");
    }
    case vm_modules::ml::VMOptimiser::DataLoaderMode::TENSOR:
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
      throw std::runtime_error("unknown dataloader type");
    }
    }
  }
};

}  // namespace serializers





bool VMGraph::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  switch mode_
  {
    case OptimiserMode::ADAGRAD:
    {
      throw std::runtime_error("serialisation not yet implemented for adagrad optimiser");
    }
    case OptimiserMode::ADAM:
    {
      throw std::runtime_error("serialisation not yet implemented for adam optimiser");
    }
    case OptimiserMode::MOMENTUM:
    {
      throw std::runtime_error("serialisation not yet implemented for momentum optimiser");
    }
    case OptimiserMode::RMSPROP:
    {
      throw std::runtime_error("serialisation not yet implemented for rmsprop optimiser");
    }
    case OptimiserMode::SGD:
    {
      optimiser
      throw std::runtime_error("serialisation not yet implemented for sgd optimiser");
    }
    default:
    {
      throw std::runtime_error("optimiser mode not recognised");
    }
  }

  buffer << optimiser_;
  return true;
}

//bool VMGraph::DeserializeFrom(serializers::MsgPackSerializer &buffer)
//{
//  fetch::ml::GraphSaveableParams<fetch::math::Tensor<fetch::vm_modules::math::DataType>> gsp;
//  buffer >> gsp;
//
//  auto vm_graph  = std::make_shared<fetch::vm_modules::ml::VMGraph>(this->vm_, this->type_id_);
//  auto graph_ptr = std::make_shared<fetch::ml::Graph<MathTensorType>>(vm_graph->GetGraph());
//  fetch::ml::utilities::BuildGraph<MathTensorType>(gsp, graph_ptr);
//
//  vm_graph->GetGraph() = *graph_ptr;
//  *this                = *vm_graph;
//
//  return true;
//}



}  // namespace fetch
