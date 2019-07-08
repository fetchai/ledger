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

#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/placeholder.hpp"
#include "python/fetch_pybind.hpp"

namespace py = pybind11;

namespace fetch {
namespace ml {

template <typename T>
void BuildGraph(std::string const &custom_name, pybind11::module &module)
{
  using ArrayType = fetch::math::Tensor<T>;
  py::class_<fetch::ml::Graph<ArrayType>>(module, custom_name.c_str())
      .def(py::init<>())
      .def("SetInput", &fetch::ml::Graph<ArrayType>::SetInput)
      .def("Evaluate", &fetch::ml::Graph<ArrayType>::Evaluate)
      .def("Backpropagate", &fetch::ml::Graph<ArrayType>::BackPropagateError)
      .def("Step", &fetch::ml::Graph<ArrayType>::Step)
      .def("StateDict", &fetch::ml::Graph<ArrayType>::StateDict)
      .def("LoadStateDict", &fetch::ml::Graph<ArrayType>::LoadStateDict)
      .def("Step",  // Convenience method to allow step without explicitly defining a FixedPoint
           [](fetch::ml::Graph<ArrayType> &g, float lr) { g.Step(T(lr)); })
      .def("AddInput",
           [](fetch::ml::Graph<ArrayType> &g, std::string const &name) {
             g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name, {});
           })
      .def("AddFullyConnected",
           [](fetch::ml::Graph<ArrayType> &g, std::string const &name, std::string const &input,
              std::size_t in, std::size_t out) {
             g.template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(name, {input}, in,
                                                                              out);
           })
      .def("AddRelu",
           [](fetch::ml::Graph<ArrayType> &g, std::string const &name, std::string const &input) {
             g.template AddNode<fetch::ml::ops::Relu<ArrayType>>(name, {input});
           })
      .def("AddSoftmax",
           [](fetch::ml::Graph<ArrayType> &g, std::string const &name, std::string const &input) {
             g.template AddNode<fetch::ml::ops::Softmax<ArrayType>>(name, {input});
           })
      .def("AddEmbeddings",
           [](fetch::ml::Graph<ArrayType> &g, std::string const &name, std::string const &input,
              uint64_t vocabSize, uint64_t embeddingsDimension) {
             g.template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(name, {input}, vocabSize,
                                                                       embeddingsDimension);
           });
}
}  // namespace ml
}  // namespace fetch
