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

#include <memory>

namespace fetch {

namespace ml {
namespace dataloaders {
template <typename LabelType, typename DataType>
class DataLoader;
}
}  // namespace ml
namespace vm_modules {
namespace math {
class VMTensor;
}
namespace ml {

class VMTrainingPair;

class VMDataLoader : public fetch::vm::Object
{
public:
  using DataType = fetch::vm_modules::math::DataType;

  VMDataLoader(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMDataLoader> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static void Bind(fetch::vm::Module &module);

  /**
   * Add data to a dataloader by passing the filenames
   * @param mode
   * @param xfilename
   * @param yfilename
   */
  void AddDataByFiles(fetch::vm::Ptr<fetch::vm::String> const &mode,
                      fetch::vm::Ptr<fetch::vm::String> const &xfilename,
                      fetch::vm::Ptr<fetch::vm::String> const &yfilename);

  /**
   * Add data to a data loader by passing in the data and labels
   * @param mode
   * @param data
   * @param labels
   */
  void AddDataByData(fetch::vm::Ptr<fetch::vm::String> const &                mode,
                     fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &data,
                     fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &labels);

  /**
   * Add data to commodity data loader
   * @param xfilename
   * @param yfilename
   */
  void AddCommodityData(fetch::vm::Ptr<fetch::vm::String> const &xfilename,
                        fetch::vm::Ptr<fetch::vm::String> const &yfilename);

  /**
   * Add data to mnist data loader
   * @param xfilename
   * @param yfilename
   */
  void AddMnistData(fetch::vm::Ptr<fetch::vm::String> const &xfilename,
                    fetch::vm::Ptr<fetch::vm::String> const &yfilename);

  /**
   * Add data to tensor data loader
   * @param xfilename
   * @param yfilename
   */
  void AddTensorData(fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &data,
                     fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &labels);

  /**
   * Get the next training pair of data and labels from the dataloader
   * @return
   */
  fetch::vm::Ptr<VMTrainingPair> GetNext();

  bool IsDone();

  std::shared_ptr<fetch::ml::dataloaders::DataLoader<fetch::math::Tensor<DataType>,
                                                     fetch::math::Tensor<DataType>>>
      loader_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
