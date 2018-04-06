#ifndef LIBFETCHCORE_MATH_BIGNUMBER_HPP
#define LIBFETCHCORE_MATH_BIGNUMBER_HPP
#include "math/bignumber.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace math
{

void BuildBigUnsigned(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<BigUnsigned, byte_array::BasicByteArray>(module, "BigUnsigned" )
    .def(py::init<  >())
    .def(py::init< const fetch::math::BigUnsigned & >())
    .def(py::init< const fetch::math::BigUnsigned::super_type & >())
    .def(py::init< const uint64_t &, std::size_t >())
    //    .def(py::self = fetch::math::BigUnsigned() )
    //    .def(py::self = byte_array::BasicByteArray() )
    //    .def(py::self = byte_array::ByteArray() )
    //    .def(py::self = byte_array::ConstByteArray() )
    .def(py::self < fetch::math::BigUnsigned() )
    .def(py::self > fetch::math::BigUnsigned() )
    .def("operator++", &BigUnsigned::operator++)
    .def("TrimmedSize", &BigUnsigned::TrimmedSize)
    .def("operator[]", &BigUnsigned::operator[])
    .def("operator<<=", &BigUnsigned::operator<<=);

}
};
};

#endif
