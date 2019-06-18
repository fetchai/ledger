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

#include "ml/dataloaders/commodity_dataloader.hpp"
#include "vm_modules/math/tensor.hpp"

namespace fetch {
  namespace vm_modules {
    namespace ml {


      class VMCommodityDataLoader : public fetch::vm::Object {
      public:
        VMCommodityDataLoader(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
            : fetch::vm::Object(vm, type_id), loader_() {}

        static void Bind(fetch::vm::Module & module) {
          module.CreateClassType<VMCommodityDataLoader>("CommodityDataLoader")
              .CreateConstuctor<>()
              .CreateMemberFunction("AddData", &VMCommodityDataLoader::AddData)
              .CreateMemberFunction("GetNext", &VMCommodityDataLoader::GetNext);
        }

        static fetch::vm::Ptr<VMCommodityDataLoader> Constructor(
            fetch::vm::VM *vm, fetch::vm::TypeId type_id) {
          return new VMCommodityDataLoader(vm, type_id);
        }

        void AddData(fetch::vm::Ptr <fetch::vm::String> const & input_filename){
          loader_.AddData(input_filename->str, input_filename->str);  // todo: this fills the output data with the input data
        }

        fetch::vm::Ptr <math::VMTensor> GetNext(){  // todo: return first and second
          auto temp = loader_.GetNext().first;  // this is a math tensor with shape (1, n_outputs)
          return this->vm_->CreateNewObject<math::VMTensor>(temp);
        }

      private:
        fetch::ml::CommodityDataLoader<fetch::math::Tensor<float>, fetch::math::Tensor<float>> loader_;
      };

    }
  }
}