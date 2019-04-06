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

#include "service/feed_subscription_manager.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace service {

void BuildFeedSubscriptionManager(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<FeedSubscriptionManager>(module, "FeedSubscriptionManager")
      .def(py::init<const fetch::service::feed_handler_type &,
                    fetch::service::AbstractPublicationFeed *>())
      .def("feed", &FeedSubscriptionManager::feed)
      .def("Subscribe", &FeedSubscriptionManager::Subscribe)
      .def("Unsubscribe", &FeedSubscriptionManager::Unsubscribe)
      .def("publisher", &FeedSubscriptionManager::publisher);
}
};  // namespace service
};  // namespace fetch
