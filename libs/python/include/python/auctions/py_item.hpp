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
      .def(py::init<>())
      .def_readwrite("id", &Item::id)
      .def_readwrite("seller_id", &Item::seller_id)
      .def_readwrite("min_price", &Item::min_price)
      .def_readwrite("max_bid", &Item::max_bid)
      .def_readwrite("sell_price", &Item::sell_price)
      .def_readwrite("bids", &Item::bids)
      .def_readwrite("bid_count", &Item::bid_count)
      .def_readwrite("winner", &Item::winner)
      .def_readwrite("agent_bid_count", &Item::agent_bid_count);


}

}  // namespace auctions
}  // namespace fetch
