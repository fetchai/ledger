#pragma once
#include "crypto/stream_hasher.hpp"

#include"fetch_pybind.hpp"

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

