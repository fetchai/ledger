#pragma once
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

