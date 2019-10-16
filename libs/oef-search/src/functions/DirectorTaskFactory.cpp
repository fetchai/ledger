#include "oef-search/functions/DirectorTaskFactory.hpp"
#include "oef-base/threading/Future.hpp"
#include "oef-base/threading/FutureCombiner.hpp"
#include "oef-search/functions/ReplyMethods.hpp"
#include "oef-messages/director.hpp"
#include "oef-messages/dap_interface.hpp"

void DirectorTaskFactory::ProcessMessageWithUri(const Uri &current_uri, ConstCharArrayBuffer &data)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Called ProcessMessage with path=", current_uri.path);

  auto                             this_sp = shared_from_this();
  std::weak_ptr<DirectorTaskFactory> this_wp = this_sp;

  if (current_uri.path == "/info")
  {
    auto response = std::make_shared<NodeInfoResponse>();
    response->set_search_key(node_config_.search_key());
    response->set_search_uri(node_config_.search_uri());
    auto future = dap_manager_->GetCoreInfo();
    future->MakeNotification().Then([future, response, current_uri, this_wp](){
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
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Can't handle path: ", current_uri.path);
    SendExceptionReply("UnknownPath", current_uri,
                       std::runtime_error("Path " + current_uri.path + " not supported!"), endpoint);
  }
}
