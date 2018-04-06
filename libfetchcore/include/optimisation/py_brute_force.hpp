#ifndef LIBFETCHCORE_OPTIMISATION_BRUTE_FORCE_HPP
#define LIBFETCHCORE_OPTIMISATION_BRUTE_FORCE_HPP
#include "optimisation/brute_force.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace optimisation
{

template< typename T >
void BuildBruteForceOptimiser(std::string const &custom_name, pybind11::module &module) {
  /*
  namespace py = pybind11;
  py::class_<BruteForceOptimiser< T >>(module, custom_name )
    .def(py::init< const std::size_t & >())
    .def("UpdateCouplingCache", &BruteForceOptimiser< T >::UpdateCouplingCache)
    .def("operator()", ( const fetch::optimisation::BruteForceOptimiser::cost_type & (BruteForceOptimiser< T >::*)(const std::size_t &, const std::size_t &) const ) &BruteForceOptimiser< T >::operator())
    .def("operator()", ( fetch::optimisation::BruteForceOptimiser::cost_type & (BruteForceOptimiser< T >::*)(const std::size_t &, const std::size_t &) ) &BruteForceOptimiser< T >::operator())
    .def("FindMinimum", ( T (BruteForceOptimiser< T >::*)() ) &BruteForceOptimiser< T >::FindMinimum)
    .def("FindMinimum", ( T (BruteForceOptimiser< T >::*)(fetch::optimisation::BruteForceOptimiser::state_type &) ) &BruteForceOptimiser< T >::FindMinimum)
    .def("CostOf", &BruteForceOptimiser< T >::CostOf)
    .def("size", &BruteForceOptimiser< T >::size);
  */
}
};
};

#endif
