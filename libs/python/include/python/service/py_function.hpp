#pragma once
#include "service/function.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {

template <typename F>
void BuildFunction(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<Function<F>>(module, custom_name).def(py::init<>()) /* No constructors found */;
}

};  // namespace service
};  // namespace fetch
