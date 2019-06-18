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
#include "vm_modules/ml/tensor.hpp"

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

        fetch::vm::Ptr <TensorWrapper> GetNext(){
          auto temp = loader_.GetNext().first;
////          auto tensor_ptr = std::make_shared<TensorWrapper>(TensorWrapper(this->vm_, this->type_id_, temp));
//          auto tensor_ptr = TensorWrapper(this->vm_, this->type_id_, temp);
//          return fetch::vm::Ptr <TensorWrapper> (&tensor_ptr);

          return this->vm_->CreateNewObject<TensorWrapper>(temp);


        }
        // Wont compile if parameter is not const &
        // The actual fetch::vm::Ptr is const, but the pointed to memory is modified
//        fetch::vm::Ptr<TrainingPairWrapper> GetData(fetch::vm::Ptr<TrainingPairWrapper> const &dataHolder) {
//          std::pair <fetch::math::Tensor<float>, std::vector<fetch::math::Tensor<float>>> d =
//              loader_.GetNext();
//          (*(dataHolder->first)).Copy(d.first);
//          (*(dataHolder->second)).Copy(d.second.at(0));
//          return dataHolder;
//        }

//        void Display(fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> const &d) {
//          loader_.Display(*d);
//        }

      private:
        fetch::ml::CommodityDataLoader<fetch::math::Tensor<float>, fetch::math::Tensor<float>> loader_;
      };

    }
  }
}