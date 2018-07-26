#pragma once

#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace byte_array {

void BuildTokenizer(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Tokenizer, std::vector<Token>>(module, "Tokenizer")
      .def(py::init<>()) /* No constructors found */
      .def("Parse", &Tokenizer::Parse)
      .def("AddConsumer", &Tokenizer::AddConsumer);
  //    .def("CreateSubspace", &Tokenizer::CreateSubspace);
}
};  // namespace byte_array
};  // namespace fetch
