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
#include "oef-search/functions/ReplyMethods.hpp"
#include "oef-search/functions/SearchTaskFactory.hpp"

void SearchTaskFactory::ProcessMessageWithUri(const Uri &current_uri, ConstCharArrayBuffer &data)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Called ProcessMessage with path=", current_uri.path);

  auto                             this_sp = shared_from_this();
  std::weak_ptr<SearchTaskFactory> this_wp = this_sp;

  if (current_uri.path == "/search")
  {
    fetch::oef::pb::SearchQuery query;
    try
    {
      IOefTaskFactory::read(query, data, data.size - data.current);
      FETCH_LOG_INFO(LOGGING_NAME, "Got search: ", query.DebugString());

      auto handle_query_result = dap_manager_->ShouldQueryBeHandled(query);
      handle_query_result->MakeNotification().Then([this_wp, handle_query_result, current_uri,
                                                    query]() mutable {
        auto sp = this_wp.lock();
        if (sp)
        {
          if (handle_query_result->get())
          {
            FETCH_LOG_INFO(LOGGING_NAME, "Query accepted! Moving to handler function..");
            sp->HandleQuery(query, current_uri);
          }
          else
          {
            FETCH_LOG_INFO(LOGGING_NAME, "Query ignored!");
            auto empty = std::make_shared<IdentifierSequence>();
            empty->mutable_status()->set_success(false);
            empty->mutable_status()->add_narrative("Ignored");
            SendReply<IdentifierSequence>("", current_uri, std::move(empty), sp->endpoint);
          }
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Failed to lock weak pointer, query cannot be executed!");
        }
        handle_query_result.reset();
      });
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "EXCEPTION: ", e.what());
      SendExceptionReply("search", current_uri, e, endpoint);
    }
  }
  else if (current_uri.path == "/update")
  {
    fetch::oef::pb::Update update;
    try
    {
      IOefTaskFactory::read(update, data, data.size - data.current);
      FETCH_LOG_INFO(LOGGING_NAME, "Got update: ", update.DebugString());
      for (auto &dmi : *(update.mutable_data_models()))
      {
        auto result_future =
            dap_manager_->parallelCall("update", *(dmi.mutable_service_description()));
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
                           "Failed to lock weak pointer, response can't be sent to agent!");
          }
        });
      }
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "EXCEPTION: ", e.what());
      SendExceptionReply("update", current_uri, e, endpoint);
    }
  }
  else if (current_uri.path == "/remove")
  {
    fetch::oef::pb::Remove remove;
    try
    {
      IOefTaskFactory::read(remove, data, data.size - data.current);
      FETCH_LOG_INFO(LOGGING_NAME, "Got remove: ", remove.DebugString());
      std::string target = "remove";
      if (remove.all())
      {
        auto        ra = remove.mutable_service_description()->add_actions();
        OEFURI::URI uri;
        uri.CoreKey = remove.key();
        uri.ParseAgent(remove.agent_key());
        uri.empty = false;
        ra->set_row_key(uri.ToString());
        ra->set_target_field_name("*");
        target = "removeRow";
      }
      auto result_future =
          dap_manager_->parallelCall(target, *remove.mutable_service_description());
      result_future->MakeNotification().Then([result_future, this_wp, current_uri]() {
        auto status = result_future->get();
        FETCH_LOG_INFO(LOGGING_NAME, "Remove status: ", status->ShortDebugString());
        auto sp = this_wp.lock();
        if (sp)
        {
          SendReply<Successfulness>("", current_uri, std::move(status), sp->endpoint);
        }
      });
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "EXCEPTION: ", e.what());
      SendExceptionReply("remove", current_uri, e, endpoint);
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

void SearchTaskFactory::HandleQuery(fetch::oef::pb::SearchQuery &query, const Uri &current_uri)
{
  auto                             this_sp = shared_from_this();
  std::weak_ptr<SearchTaskFactory> this_wp = this_sp;
  auto                             root    = std::make_shared<Branch>(query.query_v2());
  if (root->GetOperator() != "result")
  {
    auto new_root = std::make_shared<Branch>();
    new_root->SetOperator("result");
    new_root->AddBranch(std::move(root));
    root = new_root;
  }

  auto visit_future = dap_manager_->VisitQueryTreeLocal(root);

  visit_future->MakeNotification().Then([root, this_wp, current_uri, query]() mutable {
    FETCH_LOG_INFO(LOGGING_NAME, "--------------------- QUERY TREE");
    root->Print();
    FETCH_LOG_INFO(LOGGING_NAME, "---------------------");

    auto sp = this_wp.lock();
    if (sp)
    {
      sp->dap_manager_->SetQueryHeader(
          root, query, [sp, root, current_uri](fetch::oef::pb::SearchQuery &query) mutable {
            sp->ExecuteQuery(root, query, current_uri);
          });
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Query execution failed, because SearchTaskFactory weak ptr can't be locked!");
    }
  });
}

void SearchTaskFactory::ExecuteQuery(std::shared_ptr<Branch> &          root,
                                     const fetch::oef::pb::SearchQuery &query,
                                     const Uri &                        current_uri)
{
  auto                             this_sp = shared_from_this();
  std::weak_ptr<SearchTaskFactory> this_wp = this_sp;

  auto result_future = std::make_shared<fetch::oef::base::FutureCombiner<
      fetch::oef::base::FutureComplexType<std::shared_ptr<IdentifierSequence>>,
      IdentifierSequence>>();

  result_future->SetResultMerger([](std::shared_ptr<IdentifierSequence> &      results,
                                    const std::shared_ptr<IdentifierSequence> &res) {
    for (auto const &id : res->identifiers())
    {
      results->add_identifiers()->CopyFrom(id);
    }
  });

  result_future->AddFuture(dap_manager_->execute(root, query));
  result_future->AddFuture(dap_manager_->broadcast(query));

  result_future->MakeNotification().Then([result_future, this_wp, current_uri]() mutable {
    auto result = result_future->Get();
    if (result->identifiers_size() > 0)
    {
      result->mutable_status()->set_success(true);
    }
    FETCH_LOG_INFO(LOGGING_NAME, "Search response: ", result->DebugString());
    auto sp = this_wp.lock();
    if (sp)
    {
      SendReply<IdentifierSequence>("", current_uri, std::move(result), sp->endpoint);
    }
    result_future.reset();
  });
}
