#ifndef LIBFETCHCORE_STORAGE_VARIANT_STACK_HPP
#define LIBFETCHCORE_STORAGE_VARIANT_STACK_HPP
#include "storage/variant_stack.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace storage
{

void BuildVariantStack(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<VariantStack>(module, "VariantStack" )
    .def(py::init<>()) /* No constructors found */
    .def("Load", &VariantStack::Load)
    .def("Clear", &VariantStack::Clear)
    .def("Pop", &VariantStack::Pop)
    .def("New", &VariantStack::New)
    .def("Type", &VariantStack::Type)
    .def("empty", &VariantStack::empty)
    .def("size", &VariantStack::size);

}
};
};

#endif