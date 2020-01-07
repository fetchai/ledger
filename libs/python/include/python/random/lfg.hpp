#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/random/lfg.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace random {

template <std::size_t P, std::size_t Q>
void BuildLaggedFibonacciGenerator(std::string const &custom_name, pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<LaggedFibonacciGenerator<P, Q>>(module, custom_name.c_str())
      .def(py::init<>())
      .def("Reset", &LaggedFibonacciGenerator<P, Q>::Reset)
      .def("operator()", &LaggedFibonacciGenerator<P, Q>::operator())
      .def("Seed", static_cast<uint64_t (LaggedFibonacciGenerator<P, Q>::*)() const>(
                       &LaggedFibonacciGenerator<P, Q>::Seed))
      .def("Seed", static_cast<uint64_t (LaggedFibonacciGenerator<P, Q>::*)(
                       typename fetch::random::LaggedFibonacciGenerator<P, Q>::RandomType const &)>(
                       &LaggedFibonacciGenerator<P, Q>::Seed))
      .def("AsDouble", &LaggedFibonacciGenerator<P, Q>::AsDouble);
}

}  // namespace random
}  // namespace fetch
