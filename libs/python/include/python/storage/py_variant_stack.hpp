#pragma once
#include "storage/variant_stack.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace storage {

void BuildVariantStack(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<VariantStack>(module, "VariantStack")
      .def(py::init<>()) /* No constructors found */
      .def("Load", &VariantStack::Load)
      .def("Clear", &VariantStack::Clear)
      .def("Pop", &VariantStack::Pop)
      .def("New", &VariantStack::New)
      .def("Type", &VariantStack::Type)
      .def("empty", &VariantStack::empty)
      .def("size", &VariantStack::size);
}
};  // namespace storage
};  // namespace fetch
