#ifndef LIBFETCHCORE_SERIALIZER_COUNTER_HPP
#define LIBFETCHCORE_SERIALIZER_COUNTER_HPP
#include "serializers/counter.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace serializers {

template <typename S>
void BuildSizeCounter(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<SizeCounter<S>>(module, custom_name)
      .def(py::init<>()) /* No constructors found */
      .def("WriteBytes", &SizeCounter<S>::WriteBytes)
      .def("bytes_left", &SizeCounter<S>::bytes_left)
      .def("SkipBytes", &SizeCounter<S>::SkipBytes)
      .def("ReadBytes", &SizeCounter<S>::ReadBytes)
      .def("Allocate", &SizeCounter<S>::Allocate)
      .def("size", &SizeCounter<S>::size)
      .def("Seek", &SizeCounter<S>::Seek)
      .def("Tell", &SizeCounter<S>::Tell)
      .def("Reserve", &SizeCounter<S>::Reserve);
}

};  // namespace serializers
};  // namespace fetch

#endif
