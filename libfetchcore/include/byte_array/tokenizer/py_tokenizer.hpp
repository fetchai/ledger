#ifndef LIBFETCHCORE_BYTE_ARRAY_TOKENIZER_TOKENIZER_HPP
#define LIBFETCHCORE_BYTE_ARRAY_TOKENIZER_TOKENIZER_HPP
#include "byte_array/tokenizer/tokenizer.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace byte_array
{

void BuildTokenizer(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Tokenizer, std::vector<Token>>(module, "Tokenizer" )
    .def(py::init<>()) /* No constructors found */
    .def("Parse", &Tokenizer::Parse)
    .def("AddConsumer", &Tokenizer::AddConsumer);
    //    .def("CreateSubspace", &Tokenizer::CreateSubspace);

}
};
};

#endif
