#pragma once
#include "service/callable_class_member.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {

template <typename C, typename F>
void BuildCallableClassMember(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<CallableClassMember<C, F>>(module, custom_name)
      .def(py::init<>()) /* No constructors found */;
}

};  // namespace service
};  // namespace fetch
