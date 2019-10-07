#include "oef-search/dap_comms/OutboundDapConversationCreator.hpp"

#include "logging/logging.hpp"
#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/threading/StateMachineTask.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-messages/dap_interface.hpp"

#include "oef-base/conversation/OutboundConversationWorkerTask.hpp"
#include "oef-base/utils/Uri.hpp"

#include <google/protobuf/message.h>

OutboundDapConversationCreator::OutboundDapConversationCreator(size_t     thread_group_id,
                                                               const Uri &dap_uri, Core &core,
                                                               const std::string &dap_name)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Creating dap conversation with ", dap_name, " @ ",
                 dap_uri.ToString());
  worker = std::make_shared<OutboundConversationWorkerTask>(core, dap_uri, ident2conversation);

  worker->setThreadGroupId(thread_group_id);

  worker->submit();
}

OutboundDapConversationCreator::~OutboundDapConversationCreator()
{
  worker.reset();
}

std::shared_ptr<OutboundConversation> OutboundDapConversationCreator::start(
    const Uri &target_path, std::shared_ptr<google::protobuf::Message> initiator)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting dap conversation with ", target_path.host, ":",
                 target_path.port, "/", target_path.path);
  auto this_id = ident++;

  std::shared_ptr<OutboundConversation> conv;

  if (target_path.path == "execute")
  {
    conv = std::make_shared<OutboundTypedConversation<IdentifierSequence>>(this_id, target_path,
                                                                           initiator);
  }
  else if (target_path.path == "prepareConstraint" || target_path.path == "prepare")
  {
    conv = std::make_shared<OutboundTypedConversation<ConstructQueryMementoResponse>>(
        this_id, target_path, initiator);
  }
  else if (target_path.path == "calculate")
  {
    conv = std::make_shared<OutboundTypedConversation<ConstructQueryConstraintObjectRequest>>(
        this_id, target_path, initiator);
  }
  else if (target_path.path == "update")
  {
    conv = std::make_shared<OutboundTypedConversation<Successfulness>>(this_id, target_path,
                                                                       initiator);
  }
  else if (target_path.path == "describe")
  {
    conv = std::make_shared<OutboundTypedConversation<DapDescription>>(this_id, target_path,
                                                                       initiator);
  }
  else if (target_path.path == "remove" || target_path.path == "removeRow")
  {
    conv = std::make_shared<OutboundTypedConversation<Successfulness>>(this_id, target_path,
                                                                       initiator);
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Path ", target_path.path, " not supported!");
    throw std::invalid_argument(
        target_path.path + " is not a valid target, to start a OutboundDapConversationCreator!");
  }

  ident2conversation[this_id] = conv;
  worker->post(conv);
  return conv;
}
