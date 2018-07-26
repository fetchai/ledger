#pragma once
#include "serializers/type_register.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace serializers {

template <typename T>
void BuildTypeRegister(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<TypeRegister<T>>(module, custom_name)
      .def(py::init<>()) /* No constructors found */;
}
};  // namespace serializers
};  // namespace fetch
