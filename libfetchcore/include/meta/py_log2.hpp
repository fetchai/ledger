#ifndef LIBFETCHCORE_META_LOG2_HPP
#define LIBFETCHCORE_META_LOG2_HPP
#include "meta/log2.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace details
{
namespace meta
{

template< uint64_t N >
void BuildLog2(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Log2< N >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}
};
};

#endif