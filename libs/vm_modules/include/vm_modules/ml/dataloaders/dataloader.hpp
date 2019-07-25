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

#include "math/tensor.hpp"
#include "ml/dataloaders/commodity_dataloader.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "vm/module.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace fetch {
namespace vm_modules {
namespace ml {

class VMDataLoader : public fetch::vm::Object
{
  using DataType       = fetch::vm_modules::math::DataType;
  using MathTensorType = fetch::math::Tensor<DataType>;
  using VMTensorType   = fetch::vm_modules::math::VMTensor;

  using CommodityLoaderType =
      fetch::ml::dataloaders::CommodityDataLoader<fetch::math::Tensor<DataType>,
                                                  fetch::math::Tensor<DataType>>;
  using MnistLoaderType  = fetch::ml::dataloaders::MNISTLoader<fetch::math::Tensor<DataType>,
                                                              fetch::math::Tensor<DataType>>;
  using TensorLoaderType = fetch::ml::dataloaders::TensorDataLoader<fetch::math::Tensor<DataType>,
                                                                    fetch::math::Tensor<DataType>>;

public:
  VMDataLoader(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
  {}

  static fetch::vm::Ptr<VMDataLoader> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new VMDataLoader(vm, type_id);
  }

  static void Bind(fetch::vm::Module &module)
  {
    module.CreateClassType<VMDataLoader>("DataLoader")
        .CreateConstructor<>()
        .CreateMemberFunction("addData", &VMDataLoader::AddDataByFiles)
        .CreateMemberFunction("addData", &VMDataLoader::AddDataByData)
        .CreateMemberFunction("getNext", &VMDataLoader::GetNext)
        .CreateMemberFunction("isDone", &VMDataLoader::IsDone);
  }

  /**
   * Add data to a dataloader by passing the filenames
   * @param mode
   * @param xfilename
   * @param yfilename
   */
  void AddDataByFiles(fetch::vm::Ptr<fetch::vm::String> const &mode,
                      fetch::vm::Ptr<fetch::vm::String> const &xfilename,
                      fetch::vm::Ptr<fetch::vm::String> const &yfilename)
  {
    if (mode->str == "commodity")
    {
      AddCommodityData(xfilename, yfilename);
    }
    else if (mode->str == "mnist")
    {
      AddMnistData(xfilename, yfilename);
    }
    else
    {
      throw std::runtime_error("mode not valid for xfilename, yfilename input to addData");
    }
  }

  /**
   * Add data to a data loader by passing in the data and labels
   * @param mode
   * @param data
   * @param labels
   */
  void AddDataByData(fetch::vm::Ptr<fetch::vm::String> const &mode,
                     fetch::vm::Ptr<VMTensorType> const &     data,
                     fetch::vm::Ptr<VMTensorType> const &     labels)
  {
    if (mode->str == "tensor")
    {
      AddTensorData(data, labels);
    }
    else
    {
      throw std::runtime_error("mode not valid for xfilename, yfilename input to addData");
    }
  }

  /**
   * Add data to commodity data loader
   * @param xfilename
   * @param yfilename
   */
  void AddCommodityData(fetch::vm::Ptr<fetch::vm::String> const &xfilename,
                        fetch::vm::Ptr<fetch::vm::String> const &yfilename)
  {
    CommodityLoaderType loader;
    loader.AddData(xfilename->str, yfilename->str);
    loader_ = std::make_shared<CommodityLoaderType>(loader);
  }

  /**
   * Add data to mnist data loader
   * @param xfilename
   * @param yfilename
   */
  void AddMnistData(fetch::vm::Ptr<fetch::vm::String> const &xfilename,
                    fetch::vm::Ptr<fetch::vm::String> const &yfilename)
  {
    loader_ = std::make_shared<MnistLoaderType>(xfilename->str, yfilename->str);
  }

  /**
   * Add data to tensor data loader
   * @param xfilename
   * @param yfilename
   */
  void AddTensorData(fetch::vm::Ptr<VMTensorType> const &data,
                     fetch::vm::Ptr<VMTensorType> const &labels)
  {
    TensorLoaderType loader(labels->GetTensor().shape(), {data->GetTensor().shape()});
    loader.AddData(data->GetTensor(), labels->GetTensor());
    loader_ = std::make_shared<TensorLoaderType>(loader);
  }

  /**
   * Get the next training pair of data and labels from the dataloader
   * @return
   */
  fetch::vm::Ptr<VMTrainingPair> GetNext()
  {
    std::pair<fetch::math::Tensor<DataType>, std::vector<fetch::math::Tensor<DataType>>> next =
        loader_->GetNext();

    auto first      = this->vm_->CreateNewObject<math::VMTensor>(next.first);
    auto second     = this->vm_->CreateNewObject<math::VMTensor>(next.second.at(0));
    auto dataHolder = this->vm_->CreateNewObject<VMTrainingPair>(first, second);

    return dataHolder;
  }

  bool IsDone()
  {
    return loader_->IsDone();
  }

  std::shared_ptr<fetch::ml::dataloaders::DataLoader<fetch::math::Tensor<DataType>,
                                                     fetch::math::Tensor<DataType>>>
      loader_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
