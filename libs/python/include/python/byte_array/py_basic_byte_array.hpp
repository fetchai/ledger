#pragma once

#include "core/byte_array/const_byte_array.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace byte_array {

void BuildConstByteArray(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ConstByteArray>(module, "ConstByteArray")
      .def(py::init<>())
      .def(py::init<const std::size_t &>())
      .def(py::init<const char *>())
      .def(py::init<std::initializer_list<ConstByteArray::container_type>>())
      .def(py::init<const std::string &>())
      .def(py::init<const fetch::byte_array::ConstByteArray::self_type &>())
      .def(py::init<const fetch::byte_array::ConstByteArray::self_type &, const std::size_t &,
                    const std::size_t &>())
      .def("AsInt", &ConstByteArray::AsInt)
      .def(py::self != fetch::byte_array::ConstByteArray::self_type())
      //    .def(py::self != char*() )
      .def(py::self < fetch::byte_array::ConstByteArray::self_type())
      .def(py::self > fetch::byte_array::ConstByteArray::self_type())
      .def("AsFloat", &ConstByteArray::AsFloat)
      .def(py::self == fetch::byte_array::ConstByteArray::self_type())
      //    .def(py::self == char*() )
      .def(py::self + fetch::byte_array::ConstByteArray::self_type())
      //    .def("char_pointer", &ConstByteArray::char_pointer)
      //    .def("operator[]", &ConstByteArray::operator[])
      .def("SubArray", &ConstByteArray::SubArray)
      .def("capacity", &ConstByteArray::capacity)
      .def("Copy", &ConstByteArray::Copy)
      //    .def("pointer", &ConstByteArray::pointer)
      .def("Find", &ConstByteArray::Find)
      .def("Match", (bool (ConstByteArray::*)(const fetch::byte_array::ConstByteArray::self_type &,
                                              std::size_t) const) &
                        ConstByteArray::Match)
      .def("Match",
           (bool (ConstByteArray::*)(const fetch::byte_array::ConstByteArray::container_type *,
                                     std::size_t) const) &
               ConstByteArray::Match)
      .def("size", &ConstByteArray::size);
}
};  // namespace byte_array
};  // namespace fetch
