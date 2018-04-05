#ifndef LIBFETCHCORE_SERVICE_CALLABLE_CLASS_MEMBER_HPP
#define LIBFETCHCORE_SERVICE_CALLABLE_CLASS_MEMBER_HPP
#include "service/callable_class_member.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace service
{

template< typename C, typename F >
void BuildCallableClassMember(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<CallableClassMember< C, F >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}

template< typename U, typename used_args >
void BuildInvoke(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Invoke< U, used_args >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}

template< typename used_args >
void BuildUnrollArguments(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<UnrollArguments< used_args >>(module, custom_name )
    .def(py::init<>()) /* No constructors found */;

}
};
};

#endif