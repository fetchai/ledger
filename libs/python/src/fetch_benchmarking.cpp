//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "python/fetch_pybind.hpp"

#include "python/ledger/py_benchmarking.hpp"

// !!!!
namespace py = pybind11;

PYBIND11_MODULE(fetch_benchmarking, module)
{
  // Namespaces
  py::module ns_fetch_ledger = module.def_submodule("ledger");

  // Ledger
  fetch::ledger::BuildBenchmarking(ns_fetch_ledger);
}
