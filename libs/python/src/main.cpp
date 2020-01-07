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

#include "python/fetch_pybind.hpp"

#include "python/memory/array.hpp"
#include "python/memory/shared_array.hpp"

#include "python/byte_array/byte_array.hpp"
#include "python/byte_array/const_byte_array.hpp"

#include "python/random/lcg.hpp"
#include "python/random/lfg.hpp"

namespace py = pybind11;

PYBIND11_MODULE(fetch, module)
{
  // Namespaces
  py::module ns_fetch_random     = module.def_submodule("random");
  py::module ns_fetch_basic      = module.def_submodule("basic");
  py::module ns_fetch_byte_array = module.def_submodule("byte_array");
  py::module ns_fetch_serializer = module.def_submodule("serializers");

  fetch::memory::BuildArray<int8_t>("ArrayInt8", ns_fetch_basic);
  fetch::memory::BuildArray<int16_t>("ArrayInt16", ns_fetch_basic);
  fetch::memory::BuildArray<int32_t>("ArrayInt32", ns_fetch_basic);
  fetch::memory::BuildArray<int64_t>("ArrayInt64", ns_fetch_basic);

  fetch::memory::BuildArray<uint8_t>("ArrayUInt8", ns_fetch_basic);
  fetch::memory::BuildArray<uint16_t>("ArrayUInt16", ns_fetch_basic);
  fetch::memory::BuildArray<uint32_t>("ArrayUInt32", ns_fetch_basic);
  fetch::memory::BuildArray<uint64_t>("ArrayUInt64", ns_fetch_basic);

  fetch::memory::BuildArray<float>("ArrayFloat", ns_fetch_basic);
  fetch::memory::BuildArray<double>("ArrayDouble", ns_fetch_basic);

  fetch::memory::BuildSharedArray<int8_t>("SharedArrayInt8", ns_fetch_basic);
  fetch::memory::BuildSharedArray<int16_t>("SharedArrayInt16", ns_fetch_basic);
  fetch::memory::BuildSharedArray<int32_t>("SharedArrayInt32", ns_fetch_basic);
  fetch::memory::BuildSharedArray<int64_t>("SharedArrayInt64", ns_fetch_basic);

  fetch::memory::BuildSharedArray<uint8_t>("SharedArrayUInt8", ns_fetch_basic);
  fetch::memory::BuildSharedArray<uint16_t>("SharedArrayUInt16", ns_fetch_basic);
  fetch::memory::BuildSharedArray<uint32_t>("SharedArrayUInt32", ns_fetch_basic);
  fetch::memory::BuildSharedArray<uint64_t>("SharedArrayUInt64", ns_fetch_basic);

  fetch::memory::BuildSharedArray<float>("SharedArrayFloat", ns_fetch_basic);
  fetch::memory::BuildSharedArray<double>("SharedArrayDouble", ns_fetch_basic);

  fetch::byte_array::BuildConstByteArray(ns_fetch_byte_array);
  fetch::byte_array::BuildByteArray(ns_fetch_byte_array);

  fetch::random::BuildLaggedFibonacciGenerator<418, 1279>("LaggedFibonacciGenerator",
                                                          ns_fetch_random);
  fetch::random::BuildLinearCongruentialGenerator(ns_fetch_random);
}
