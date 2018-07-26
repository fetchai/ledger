#pragma once
#include "fetch_pybind.hpp"
#include "storage/random_access_stack.hpp"

namespace fetch {
namespace storage {

template <typename T, typename D>
void BuildRandomAccessStack(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<RandomAccessStack<T, D>>(module, custom_name)
      .def(py::init<>()) /* No constructors found */
      .def("Load", &RandomAccessStack<T, D>::Load)
      .def("header_extra", &RandomAccessStack<T, D>::header_extra)
      .def("Set", &RandomAccessStack<T, D>::Set)
      .def("Get", &RandomAccessStack<T, D>::Get)
      .def("Top", &RandomAccessStack<T, D>::Top)
      .def("Pop", &RandomAccessStack<T, D>::Pop)
      .def("SetExtraHeader", &RandomAccessStack<T, D>::SetExtraHeader)
      .def("Swap", &RandomAccessStack<T, D>::Swap)
      .def("Push", &RandomAccessStack<T, D>::Push)
      .def("New", &RandomAccessStack<T, D>::New)
      .def("Clear", &RandomAccessStack<T, D>::Clear)
      .def("empty", &RandomAccessStack<T, D>::empty)
      .def("size", &RandomAccessStack<T, D>::size);
}
};  // namespace storage
};  // namespace fetch
