#ifndef LIBFETCHCORE_SERIALIZER_COUNTER_HPP
#define LIBFETCHCORE_SERIALIZER_COUNTER_HPP
#include "serializer/counter.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace serializers
{

void BuildTypedByte_ArrayBuffer(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<TypedByte_ArrayBuffer>(module, "TypedByte_ArrayBuffer" )
    .def(py::init<>()) /* No constructors found */;

}

template< typename S >
void BuildSizeCounter(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<SizeCounter< S >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */
    .def("WriteBytes", &SizeCounter< S >::WriteBytes)
    .def("bytes_left", &SizeCounter< S >::bytes_left)
    .def("SkipBytes", &SizeCounter< S >::SkipBytes)
    .def("ReadBytes", &SizeCounter< S >::ReadBytes)
    .def("Allocate", &SizeCounter< S >::Allocate)
    .def("size", &SizeCounter< S >::size)
    .def("Seek", &SizeCounter< S >::Seek)
    .def("Tell", &SizeCounter< S >::Tell)
    .def("Reserve", &SizeCounter< S >::Reserve);

}

void BuildSizeCounter(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<SizeCounter>(module, "SizeCounter" )
    .def(py::init<>()) /* No constructors found */
    .def("WriteBytes", &SizeCounter::WriteBytes)
    .def("bytes_left", &SizeCounter::bytes_left)
    .def("SkipBytes", &SizeCounter::SkipBytes)
    .def("ReadBytes", &SizeCounter::ReadBytes)
    .def("Allocate", &SizeCounter::Allocate)
    .def("size", &SizeCounter::size)
    .def("Seek", &SizeCounter::Seek)
    .def("Tell", &SizeCounter::Tell)
    .def("Reserve", &SizeCounter::Reserve);

}
};
};

#endif