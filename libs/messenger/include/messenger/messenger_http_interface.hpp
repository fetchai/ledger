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

#include "http/json_response.hpp"
#include "http/module.hpp"
#include "messenger/messenger_api.hpp"

namespace fetch {
namespace messenger {

class MessengerHttpModule : public http::HTTPModule
{
public:
  MessengerHttpModule(MessengerAPI &messenger)
    : messenger_{messenger}
  {

    Post("/api/messenger/register", "Registers an agent to the network.",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           // Finding sender
           auto doc = request.JSON();
           if (!doc.Has("sender"))  // TODO: Check that it is a string as well.
           {
             return http::CreateJsonResponse("", http::Status::CLIENT_ERROR_BAD_REQUEST);
           }

           // Sender
           service::CallContext context;
           context.sender_address =
               byte_array::FromBase64(doc["sender"].As<byte_array::ConstByteArray>());

           // TODO: HTTP messenger must always have a mailbox
           bool setup_mailbox = doc["mailbox"].As<bool>();

           // TODO: Authentication missing.
           messenger_.RegisterMessenger(context, setup_mailbox);

           variant::Variant response = variant::Variant::Object();
           response["status"]        = "OK";
           return http::CreateJsonResponse(response);
         });

    Post("/api/messenger/unregister", "Unregisters an agent from the network.",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           // Finding sender
           auto doc = request.JSON();
           if (!doc.Has("sender"))  // TODO: Check that it is a string as well.
           {
             return http::CreateJsonResponse("", http::Status::CLIENT_ERROR_BAD_REQUEST);
           }

           // Sender
           service::CallContext context;
           context.sender_address =
               byte_array::FromBase64(doc["sender"].As<byte_array::ConstByteArray>());

           // TODO: Authentication missing.
           messenger_.UnregisterMessenger(context);

           variant::Variant response = variant::Variant::Object();
           response["status"]        = "OK";
           return http::CreateJsonResponse(response);
         });

    Post("/api/messenger/sendmessage", "Sends a message to a specific agent.",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           // Finding sender
           auto doc = request.JSON();
           if (!doc.Has("sender"))  // TODO: Check that it is a string as well.
           {
             return http::CreateJsonResponse("", http::Status::CLIENT_ERROR_BAD_REQUEST);
           }

           if (!doc.Has("message"))  // TODO: Check that it is a string as well.
           {
             return http::CreateJsonResponse("", http::Status::CLIENT_ERROR_BAD_REQUEST);
           }

           // Sender
           service::CallContext context;
           context.sender_address =
               byte_array::FromBase64(doc["sender"].As<byte_array::ConstByteArray>());

           serializers::MsgPackSerializer buffer{
               byte_array::FromBase64(doc["message"].As<byte_array::ConstByteArray>())};
           Message msg;
           buffer >> msg;

           // TODO: Authentication missing.
           messenger_.SendMessage(context, msg);

           variant::Variant response = variant::Variant::Object();
           response["status"]        = "OK";
           return http::CreateJsonResponse(response);
         });

    Post("/api/messenger/getmessages", "Gets messages in inbox.",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           // Finding sender
           auto doc = request.JSON();
           if (!doc.Has("sender"))  // TODO: Check that it is a string as well.
           {
             return http::CreateJsonResponse("", http::Status::CLIENT_ERROR_BAD_REQUEST);
           }

           // Sender
           service::CallContext context;
           context.sender_address =
               byte_array::FromBase64(doc["sender"].As<byte_array::ConstByteArray>());

           // TODO: Authentication missing.
           auto messages = messenger_.GetMessages(context);

           // Creating stream with messages
           serializers::MsgPackSerializer buffer;
           buffer << messages;

           variant::Variant response = variant::Variant::Object();
           response["status"]        = "OK";
           response["messages"]      = byte_array::ToBase64(buffer.data());
           return http::CreateJsonResponse(response);
         });

    Post("/api/messenger/findagent", "Finds agents matching search criteria.",
         [](http::ViewParameters const &, http::HTTPRequest const &) {
           //  ResultList FindAgents(service::CallContext const & call_context, ConstByteArray const
           //  & query_type, ConstByteArray const & query);

           return http::CreateJsonResponse("{}");
         });

    Post("/api/messenger/advertise", "Creates advertisement on the node.",
         [](http::ViewParameters const &, http::HTTPRequest const &) {
           //   void Advertise(service::CallContext const & call_context);
           return http::CreateJsonResponse("{}");
         });
  }

private:
  MessengerAPI &messenger_;
};
}  // namespace messenger
}  // namespace fetch
