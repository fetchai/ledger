#ifndef LIBFETCHCORE_BYTE_ARRAY_TOKENIZER_TOKEN_HPP
#define LIBFETCHCORE_BYTE_ARRAY_TOKENIZER_TOKEN_HPP
#include "byte_array/tokenizer/token.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace byte_array
{

void BuildToken(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<Token, fetch::byte_array::ConstByteArray>(module, "Token" )
    .def(py::init<  >())
    .def(py::init< const char * >())
    .def(py::init< const std::string & >())
    .def(py::init< const fetch::byte_array::ConstByteArray & >())
    .def(py::init< const fetch::byte_array::ConstByteArray &, const std::size_t &, const std::size_t & >())
    .def("character", &Token::character)
    .def("SetFilename", &Token::SetFilename)
    .def("filename", &Token::filename)
    .def("SetChar", &Token::SetChar)
    .def("SetLine", &Token::SetLine)
    .def("line", &Token::line)
    .def("type", &Token::type)
    .def("SetType", &Token::SetType);

}
};
};

#endif