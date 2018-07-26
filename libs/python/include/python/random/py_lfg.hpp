#pragma once

#include"core/random/lfg.hpp"
#include"python/fetch_pybind.hpp"

namespace fetch
{
namespace random
{

template< std::size_t P, std::size_t Q >
void BuildLaggedFibonacciGenerator(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<LaggedFibonacciGenerator< P, Q >>(module, custom_name.c_str() )
    .def(py::init<  >())
    .def("Reset", &LaggedFibonacciGenerator< P, Q >::Reset)
    .def("operator()", &LaggedFibonacciGenerator< P, Q >::operator())
    .def("Seed", ( uint64_t (LaggedFibonacciGenerator< P, Q >::*)() const ) &LaggedFibonacciGenerator< P, Q >::Seed)
    .def("Seed", ( uint64_t (LaggedFibonacciGenerator< P, Q >::*)(const typename fetch::random::LaggedFibonacciGenerator<P, Q>::random_type &) ) &LaggedFibonacciGenerator< P, Q >::Seed)
    .def("AsDouble", &LaggedFibonacciGenerator< P, Q >::AsDouble);

}
};
};

