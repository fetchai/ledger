#ifndef LIBFETCHCORE_MATH_EXP_HPP
#define LIBFETCHCORE_MATH_EXP_HPP
#include "math/exp.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace math
{

template< uint8_t N, uint64_t C, bool O >
void BuildExp(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Exp< N, C, O >>(module, custom_name.c_str() )
    .def(py::init< const Exp<N, C, O> & >())
    .def(py::init<  >())
    //    .def(py::self = py::self )
    .def("SetCoefficient", &Exp< N, C, O >::SetCoefficient);

}
};
};

#endif
