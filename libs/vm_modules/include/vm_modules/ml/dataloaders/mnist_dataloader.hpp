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

#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/training_pair.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {

class MnistDataLoader : public fetch::vm::Object
{
public:
  MnistDataLoader(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &images_file,
                  std::string const &labels_file)
    : fetch::vm::Object(vm, type_id)
    , loader_(images_file, labels_file)
  {}

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<fetch::vm_modules::ml::MnistDataLoader>("MNISTLoader")
        .CreateConstuctor<fetch::vm::Ptr<fetch::vm::String>, fetch::vm::Ptr<fetch::vm::String>>()
        .CreateMemberFunction("GetData", &fetch::vm_modules::ml::MnistDataLoader::GetData)
        .CreateMemberFunction("Display", &fetch::vm_modules::ml::MnistDataLoader::Display);
  }

  static fetch::vm::Ptr<MnistDataLoader> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::String> const &images_file,
      fetch::vm::Ptr<fetch::vm::String> const &labels_file)
  {
    return new MnistDataLoader(vm, type_id, images_file->str, labels_file->str);
  }

  // Wont compile if parameter is not const &
  // The actual fetch::vm::Ptr is const, but the pointed to memory is modified
  fetch::vm::Ptr<TrainingPair> GetData(fetch::vm::Ptr<TrainingPair> const &dataHolder)
  {
    std::pair<fetch::math::Tensor<float>, std::vector<fetch::math::Tensor<float>>> d =
        loader_.GetNext();
    (*(dataHolder->first)).Copy(d.first);
    (*(dataHolder->second)).Copy(d.second.at(0));
    return dataHolder;
  }

  void Display(fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &d)
  {
    loader_.Display((*d).GetTensor());
  }

  fetch::ml::dataloaders::MNISTLoader<fetch::math::Tensor<float>, fetch::math::Tensor<float>>
      loader_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
