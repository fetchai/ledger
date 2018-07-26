#ifndef LIBFETCHCORE_MEMORY_RANGE_HPP
#define LIBFETCHCORE_MEMORY_RANGE_HPP

#include "math/rectangular_array.hpp"
#include "python/fetch_pybind.hpp"
#include "vectorise/memory/range.hpp"

namespace fetch {
namespace memory {

void BuildRange(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<Range>(module, custom_name.c_str())
      .def(py::init<const std::size_t &, const std::size_t &>())
      .def("from", &Range::from)
      .def("to", &Range::to);
}

};  // namespace memory
};  // namespace fetch

#endif
