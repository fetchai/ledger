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

#include "protocols/chain_keeper/protocol.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace protocols {

void BuildChainKeeperProtocol(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ChainKeeperProtocol, fetch::protocols::ChainKeeperController, fetch::service::Protocol,
             fetch::http::HTTPModule>(module, "ChainKeeperProtocol")
      .def(py::init<network::NetworkManager *, const uint64_t &, fetch::protocols::EntryPoint &>())
      .def("Ping", &ChainKeeperProtocol::Ping);
}
};  // namespace protocols
};  // namespace fetch
