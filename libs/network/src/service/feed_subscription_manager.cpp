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

#include "network/service/feed_subscription_manager.hpp"

#include "network/service/server_interface.hpp"

namespace fetch {
namespace service {

void FeedSubscriptionManager::PublishingProcessor()
{
  std::vector<publishing_workload_type> my_work;
  publishing_workload_.Get(my_work, 16);
  std::list<std::tuple<service_type *, connection_handle_type>> dead_connections;
  for (auto &w : my_work)
  {
    service_type *         service       = std::get<0>(w);
    connection_handle_type client_number = std::get<1>(w);
    network::message_type  msg           = std::get<2>(w);
    if (!service->DeliverResponse(client_number, msg.Copy()))
    {
      dead_connections.push_back(std::make_tuple(service, client_number));
    }
  }
  if (!dead_connections.empty())
  {
    for (auto &w : dead_connections)
    {
      service_type *         service       = std::get<0>(w);
      connection_handle_type client_number = std::get<1>(w);
      service->ConnectionDropped(client_number);
    }
  }

  if (publishing_workload_.Remaining())
  {
    workers_->Post([this]() { this->PublishingProcessor(); });
  }
}

void FeedSubscriptionManager::AttachToService(ServiceServerInterface *service)
{
  LOG_STACK_TRACE_POINT;

  auto feed = feed_;
  publisher_->create_publisher(feed_,
                               [service, feed, this](fetch::byte_array::ConstByteArray const &msg) {
                                 serializer_type params;
                                 params << SERVICE_FEED << feed;

                                 uint64_t p = params.tell();
                                 params << subscription_handler_type(0);  // placeholder

                                 params.Allocate(msg.size());
                                 params.WriteBytes(msg.pointer(), msg.size());
                                 LOG_STACK_TRACE_POINT;
                                 lock_type lock(subscribe_mutex_);

                                 std::vector<publishing_workload_type> notifications_to_send;
                                 notifications_to_send.reserve(16);
                                 std::size_t i = 0;

                                 while (i < subscribers_.size())
                                 {
                                   auto &s = subscribers_[i];
                                   params.seek(p);
                                   params << s.id;

                                   publishing_workload_type new_notification =
                                       std::make_tuple<>(service, s.client, params.data());
                                   notifications_to_send.push_back(new_notification);

                                   i++;
                                   if ((i & 0xF) == 0)
                                   {
                                     PublishAll(notifications_to_send);
                                   }
                                 }
                                 PublishAll(notifications_to_send);
                               });
}

}  // namespace service
}  // namespace fetch
