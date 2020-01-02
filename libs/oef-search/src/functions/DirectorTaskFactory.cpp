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

#include "oef-base/threading/Future.hpp"
#include "oef-base/threading/FutureCombiner.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-messages/director.hpp"
#include "oef-search/functions/DirectorTaskFactory.hpp"
#include "oef-search/functions/ReplyMethods.hpp"

void DirectorTaskFactory::ProcessMessageWithUri(const Uri &current_uri, ConstCharArrayBuffer &data)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Called ProcessMessage with path=", current_uri.path);

  auto                               this_sp = shared_from_this();
  std::weak_ptr<DirectorTaskFactory> this_wp = this_sp;

  if (current_uri.path == "/info")
  {
    auto response = std::make_shared<NodeInfoResponse>();
    response->set_search_key(node_config_.search_key());
    response->set_search_uri(node_config_.search_uri());
    auto future = dap_manager_->GetCoreInfo();
    future->MakeNotification().Then([future, response, current_uri, this_wp]() {
      auto core = future->get();
      response->set_core_key(core.first);
      response->set_core_uri(core.second);
      auto sp = this_wp.lock();
      if (sp)
      {
        SendReply<NodeInfoResponse>("", current_uri, std::move(response), sp->endpoint);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME,
                       "Failed to lock weak pointer, response can't be sent to director!");
      }
    });
  }
  else if (current_uri.path == "/location")
  {
    Actions update;
    try
    {
      IOefTaskFactory::read(update, data, data.size - data.current);
      FETCH_LOG_INFO(LOGGING_NAME, "Got location update: ", update.DebugString());

      auto result_future = dap_manager_->parallelCall("update", update);
      result_future->MakeNotification().Then([result_future, this_wp, current_uri]() {
        auto status = result_future->get();
        FETCH_LOG_INFO(LOGGING_NAME, "Update status: ", status->ShortDebugString());
        auto sp = this_wp.lock();
        if (sp)
        {
          SendReply<Successfulness>("", current_uri, std::move(status), sp->endpoint);
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME,
                         "Failed to lock weak pointer, response can't be sent to director!");
        }
      });
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "EXCEPTION: ", e.what());
      SendExceptionReply("update", current_uri, e, endpoint);
    }
  }
  else if (current_uri.path == "/peer")
  {
    Actions update;
    try
    {
      IOefTaskFactory::read(update, data, data.size - data.current);
      FETCH_LOG_INFO(LOGGING_NAME, "Got peer: ", update.ShortDebugString());

      uint16_t count = 0;
      for (const auto &upd : update.actions())
      {
        if (upd.operator_() == "ADD_PEER" && upd.query_field_type() == "address" &&
            !upd.query_field_value().s().empty())
        {
          peers_->AddPeer(upd.query_field_value().s());
        }
        else
        {
          ++count;
          FETCH_LOG_WARN(LOGGING_NAME, "Got invalid peer update: ", upd.ShortDebugString());
        }
      }

      auto status = std::make_shared<Successfulness>();
      status->set_success(count == 0);
      SendReply<Successfulness>("", current_uri, std::move(status), endpoint);
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "EXCEPTION: ", e.what());
      SendExceptionReply("update", current_uri, e, endpoint);
    }
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Can't handle path: ", current_uri.path);
    SendExceptionReply("UnknownPath", current_uri,
                       std::runtime_error("Path " + current_uri.path + " not supported!"),
                       endpoint);
  }
}
