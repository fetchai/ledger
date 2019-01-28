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

#include "auctions/item.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace auctions {

void BuildItem(std::string const &custom_name, pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Item>(module, custom_name.c_str())
      .def(py::init<ItemId, AgentId, Value>())
      .def("Id", [](Item &item) { return item.id; })
      .def("SellerId", [](Item &item) { return item.seller_id; })
      .def("MinPrice", [](Item &item) { return item.min_price; })
      .def("MaxBid", [](Item &item) { return item.max_bid; })
      .def("SellPrice", [](Item &item) { return item.sell_price; })
      .def("Bids", [](Item &item) { return item.bids; })
      .def("BidCount", [](Item &item) { return item.bid_count; })
      .def("Winner", [](Item &item) { return item.winner; })
      .def("AgentBidCount", [](Item &item) { return item.agent_bid_count; });
}

}  // namespace auctions
}  // namespace fetch
