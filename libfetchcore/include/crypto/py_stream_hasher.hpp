#ifndef LIBFETCHCORE_CRYPTO_STREAM_HASHER_HPP
#define LIBFETCHCORE_CRYPTO_STREAM_HASHER_HPP
#include "crypto/stream_hasher.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace crypto
{

void BuildStreamHasher(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<StreamHasher>(module, "StreamHasher" )
    .def(py::init<>()) /* No constructors found */;

}
};
};

#endif