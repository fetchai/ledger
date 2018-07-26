#pragma once

#include "core/byte_array/tokenizer/token.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace byte_array {

void BuildToken(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Token, fetch::byte_array::ConstByteArray>(module, "Token")
      .def(py::init<>())
      .def(py::init<const char *>())
      .def(py::init<const std::string &>())
      .def(py::init<const fetch::byte_array::ConstByteArray &>())
      .def(py::init<const fetch::byte_array::ConstByteArray &,
                    const std::size_t &, const std::size_t &>())
      .def("character", &Token::character)
      .def("SetChar", &Token::SetChar)
      .def("SetLine", &Token::SetLine)
      .def("line", &Token::line)
      .def("type", &Token::type)
      .def("SetType", &Token::SetType);
}
};  // namespace byte_array
};  // namespace fetch
