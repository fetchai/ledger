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
#include "auctions/error_codes.hpp"
#include "math/tensor.hpp"

#include "python/fetch_pybind.hpp"

namespace fetch {
namespace auctions {

void BuildCombinatorialAuction(std::string const &custom_name, pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<CombinatorialAuction>(module, custom_name.c_str())
      .def(py::init<>())
      .def("AddItem",
           [](CombinatorialAuction &ca, Item const &item) {
             ErrorCode ec;
             ec = ca.AddItem(item);

             if (ec == ErrorCode::SUCCESS)
             {
               return 0;
             }
             else
             {
               return 1;
             }
           })
      .def("ShowListedItems", [](CombinatorialAuction const &ca) { return ca.ShowListedItems(); })
      .def("ShowBids", [](CombinatorialAuction const &ca) { return ca.ShowBids(); })
      .def("PlaceBid",
           [](CombinatorialAuction &ca, fetch::auctions::Bid bid) {
             ErrorCode ec;
             ec = ca.PlaceBid(bid);

             if (ec == ErrorCode::SUCCESS)
             {
               return 0;
             }
             else
             {
               return 1;
             }
           })
      .def("Mine", [](CombinatorialAuction &ca, std::size_t seed,
                      std::size_t run_len) { ca.Mine(seed, run_len); })
      .def("Couplings", &CombinatorialAuction::Couplings)
      .def("LocalFields", &CombinatorialAuction::LocalFields)
      .def("TotalBenefit", &CombinatorialAuction::TotalBenefit)
      .def("SelectBid", &CombinatorialAuction::SelectBid)
      .def("BuildGraph", &CombinatorialAuction::BuildGraph)
      .def("Execute", &CombinatorialAuction::Execute);
}

}  // namespace auctions
}  // namespace fetch
