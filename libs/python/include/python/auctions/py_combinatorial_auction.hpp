#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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


#include "core/byte_array/byte_array.hpp"
#include "auctions/combinatorial_auction.hpp"

#include "python/fetch_pybind.hpp"


namespace fetch {
namespace auctions {

void BuildCombinatorialAuction(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<CombinatorialAuction>(module, custom_name.c_str())
//      .def(py::init<>())
      .def(py::init<BlockIdType, BlockIdType>())
//      .def("Copy",
//           []() {
//             ShapelessArray<T> a;
//             return a.Copy();
//           })
      .def("AddItem", [](CombinatorialAuction & ca, fetch::auctions::Item const &item)
      {
        return ca.AddItem(item);
      })
      .def("Bid", [](CombinatorialAuction & ca, fetch::auctions::Bid bid)
      {
        return ca.Bid(bid);
      })
      .def("Mine", (CombinatorialAuction & (CombinatorialAuction::*)(std::size_t random_seed, std::size_t run_time)) &
                            CombinatorialAuction::Mine)
      .def("Execute", (CombinatorialAuction & (CombinatorialAuction::*)(std::size_t block_id)) &
                            CombinatorialAuction::Execute);
}

}  // namespace math
}  // namespace fetch
