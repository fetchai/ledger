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

#include "ml/dataloaders/word2vec_loaders/cbow_dataloader.hpp"
#include "python/fetch_pybind.hpp"

namespace py = pybind11;

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename T>
void BuildCBOWLoader(std::string const &custom_name, pybind11::module &module)
{
  using ArrayType = fetch::math::Tensor<T>;
  py::class_<fetch::ml::dataloaders::CBoWLoader<ArrayType>>(module, custom_name.c_str())
      .def(py::init<fetch::ml::dataloaders::CBoWTextParams<ArrayType>>())
      .def("AddData", &fetch::ml::dataloaders::CBoWLoader<ArrayType>::AddData)
      .def("Size", &fetch::ml::dataloaders::CBoWLoader<ArrayType>::Size)
      .def("IsDone", &fetch::ml::dataloaders::CBoWLoader<ArrayType>::IsDone)
      .def("Reset", &fetch::ml::dataloaders::CBoWLoader<ArrayType>::Reset)
      .def("GetNext", &fetch::ml::dataloaders::CBoWLoader<ArrayType>::GetNext)
      .def("GetVocab", &fetch::ml::dataloaders::CBoWLoader<ArrayType>::GetVocab)
      .def("VocabSize", &fetch::ml::dataloaders::CBoWLoader<ArrayType>::VocabSize);
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
