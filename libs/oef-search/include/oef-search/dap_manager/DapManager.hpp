#pragma once

#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/threading/Future.hpp"
#include "oef-base/threading/Waitable.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-messages/search_query.hpp"
#include "oef-search/dap_comms/DapConversationTask.hpp"
#include "oef-search/dap_comms/DapParallelConversationTask.hpp"
#include "oef-search/dap_manager/BranchExecutorTask.hpp"
#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/NodeExecutorFactory.hpp"
#include "oef-search/dap_manager/IdCache.hpp"
#include "oef-search/search_comms/SearchPeerStore.hpp"
#include "visitors/AddMoreDapsBasedOnOptionsVisitor.hpp"
#include "visitors/CollectDapsVisitor.hpp"
#include "visitors/FindGeoLocationVisitor.hpp"
#include "visitors/PopulateActionsVisitorDescentPass.hpp"
#include "visitors/PopulateFieldInformationVisitor.hpp"
#include <atomic>
#include <memory>
#include <mutex>

class DapManager : public std::enable_shared_from_this<DapManager>
{
public:
  static constexpr char const *LOGGING_NAME = "DapManager";

  DapManager(std::shared_ptr<DapStore>              dap_store,
             std::shared_ptr<SearchPeerStore>       search_peer_store,
             std::shared_ptr<OutboundConversations> outbounds,
             uint64_t query_cache_lifetime_sec)
    : dap_store_{std::move(dap_store)}
    , search_peer_store_{std::move(search_peer_store)}
    , outbounds_{std::move(outbounds)}
    , query_id_cache_{std::make_shared<IdCache>(query_cache_lifetime_sec, query_cache_lifetime_sec)}
  {
    query_id_cache_->submit();
  }
  virtual ~DapManager() = default;

  void setup()
  {
    uint32_t                  msg_id          = 1110;
    auto                      initiator_proto = std::make_shared<NoInputParameter>();
    auto                      this_sp         = this->shared_from_this();
    std::weak_ptr<DapManager> this_wp         = this_sp;
    for (const auto &dap : dap_store_->GetDaps())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "CALL DESCRIBE FOR: ", dap);
      auto convTask = std::make_shared<DapConversationTask<NoInputParameter, DapDescription>>(
          dap, "describe", ++msg_id, initiator_proto, outbounds_);
      convTask->messageHandler = [this_wp, &dap](std::shared_ptr<DapDescription> response) {
        FETCH_LOG_INFO(LOGGING_NAME, "Got DAP describe: ", response->DebugString());
        auto sp = this_wp.lock();
        if (sp)
        {
          sp->dap_store_->ConfigureDap(dap, *response);
        }
        else
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to DapParallelConversationTask");
        }
      };
      convTask->submit();
    }
  }

  std::shared_ptr<FutureComplexType<std::shared_ptr<Successfulness>>> parallelCall(
      const std::string &path, Actions &update)
  {
    auto future = std::make_shared<FutureComplexType<std::shared_ptr<Successfulness>>>();

    auto convTask = std::make_shared<
        DapParallelConversationTask<ConstructQueryConstraintObjectRequest, Successfulness>>(
        parallel_call_msg_id, outbounds_);

    for (auto &upd : *update.mutable_actions())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Handling ", path, ": ", upd.DebugString());
      dap_store_->UpdateTargetFieldAndTableNames(upd);
      auto daps = dap_store_->GetDapsForAttributeName(upd.target_field_name());
      if (daps.empty())
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "No DAPs claimed this value -- ", upd.target_field_name());
        continue;
      }

      auto upd_pt = std::make_shared<ConstructQueryConstraintObjectRequest>(upd);
      for (const auto &dap : daps)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Sending ", path, " to: ", dap, upd_pt->DebugString());
        convTask->Add(DapInputDataType<ConstructQueryConstraintObjectRequest>{dap, path, upd_pt});
        ++parallel_call_msg_id;
      }
    }
    convTask->submit();

    convTask->MakeNotification().Then([future, convTask, path]() {
      auto status = std::make_shared<Successfulness>();
      status->set_success(true);
      FETCH_LOG_INFO(LOGGING_NAME, "convTask done");
      for (const auto &res : convTask->GetOutputs())
      {
        if (!res->success())
        {
          status->set_success(false);
          FETCH_LOG_WARN(LOGGING_NAME, "DAP returned error messages when calling ", path, ": ");
          for (const auto &m : res->narrative())
          {
            status->add_narrative(m);
            FETCH_LOG_WARN(LOGGING_NAME, "--> ", m);
          }
        }
      }
      future->set(std::move(status));
    });

    return future;
  }

  template <typename IN_PROTO, typename OUT_PROTO>
  std::shared_ptr<FutureComplexType<std::shared_ptr<OUT_PROTO>>> SingleDapCall(
      std::string const &dap_name, std::string const &path, std::shared_ptr<IN_PROTO> in_proto)
  {
    auto future = std::make_shared<FutureComplexType<std::shared_ptr<OUT_PROTO>>>();

    auto convTask = std::make_shared<DapConversationTask<IN_PROTO, OUT_PROTO>>(
        dap_name, path, ++single_call_msg_id, in_proto, outbounds_);

    convTask->submit();

    std::weak_ptr<FutureComplexType<std::shared_ptr<OUT_PROTO>>> future_wp = future;

    convTask->messageHandler = [future_wp](std::shared_ptr<OUT_PROTO> response) {
      auto sp = future_wp.lock();
      if (sp)
      {
        sp->set(std::move(response));
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to Future");
      }
    };

    convTask->errorHandler = [future_wp](const std::string &dap_name, const std::string &path,
                                         const std::string &msg) {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to call ", dap_name, " with path: ", path, ": ", msg);
      auto sp = future_wp.lock();
      if (sp)
      {
        sp->set(nullptr);
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to Future");
      }
    };

    return future;
  }

  std::shared_ptr<OutboundConversations> &GetOutbounds()
  {
    return outbounds_;
  }

  std::shared_ptr<DapStore> &GetDapStore()
  {
    return dap_store_;
  }

  std::size_t GetNewSingleDapCallId()
  {
    return ++single_call_msg_id;
  }

  std::size_t GetNewSerialCallId()
  {
    return ++serial_call_msg_id;
  }

  std::shared_ptr<Future<bool>> ShouldQueryBeHandled(fetch::oef::pb::SearchQuery &query)
  {
    auto result = std::make_shared<Future<bool>>();

    if (query.ttl() > 1024)
    {
      result->set(false);
    }
    else if (query_id_cache_->IsCached(query.id()))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Query cached, will be ignored!");
      result->set(false);
    }
    else if (query.has_directed_search() && query.directed_search().has_target())
    {
      if (query.directed_search().has_distance())
      {
        bool has_geo   = query.directed_search().target().has_geo();
        bool has_topic = query.directed_search().target().has_topic();
        if (has_geo)
        {
          PlaneDistanceCheck("geo", query.directed_search(), result);
        }
        if (has_topic)
        {
          // TODO: topic handling
          FETCH_LOG_WARN(LOGGING_NAME, "Query is topic directed, which is not yet implemented! ");
          result->set(true);
        }
        if (!has_geo && !has_topic)
        {
          result->set(false);
        }
      }
      else
      {
        // if the search is targeted, but no previous distance is set don't handle the query
        result->set(false);
      }
    }
    else
    {
      result->set(true);
    }

    return result;
  }

  std::shared_ptr<FutureComplexType<std::shared_ptr<IdentifierSequence>>> execute(
      std::shared_ptr<Branch> root)
  {
    auto result    = std::make_shared<FutureComplexType<std::shared_ptr<IdentifierSequence>>>();
    auto visit_res = VisitQueryTreeNetwork(root);

    auto identifier_sequence = std::make_shared<IdentifierSequence>();
    identifier_sequence->set_originator(true);

    std::shared_ptr<DapManager> this_sp = shared_from_this();

    visit_res->MakeNotification().Then([result, root, identifier_sequence, this_sp]() mutable {
      FETCH_LOG_INFO(LOGGING_NAME, "--------------------- AFTER VISIT");
      root->Print();
      FETCH_LOG_INFO(LOGGING_NAME, "---------------------");

      auto execute_task =
          NodeExecutorFactory(BranchExecutorTask::NodeDataType(root),
                              identifier_sequence, this_sp);

      execute_task->SetMessageHandler([result](std::shared_ptr<IdentifierSequence> response) {
        response->mutable_status()->set_success(true);
        result->set(std::move(response));
      });

      execute_task->submit();
    });

    return result;
  }

  std::shared_ptr<FutureComplexType<std::shared_ptr<IdentifierSequence>>> broadcast(
      std::shared_ptr<Branch> root, const fetch::oef::pb::SearchQuery &query)
  {
    query_id_cache_->Add(query.id());

    auto result  = std::make_shared<FutureComplexType<std::shared_ptr<IdentifierSequence>>>();
    auto this_sp = this->shared_from_this();
    std::weak_ptr<DapManager> this_wp = this_sp;
    FETCH_LOG_INFO(LOGGING_NAME, "Broadcast started");

    if (!query.has_directed_search() || !query.directed_search().has_target() ||
        !query.directed_search().target().has_geo())
    {
      // location lookup
      FETCH_LOG_INFO(LOGGING_NAME,
                     "No location set in header, looking for location constraint in the query...");
      auto v = std::make_shared<FindGeoLocationVisitor>(dap_store_);
      v->SubmitVisitTask(root);
      v->MakeNotification().Then([v, query, result, this_wp]() mutable {
        auto loc_res = v->GetLocationRoot();
        if (loc_res)
        {
          // set location
          auto new_query = std::make_shared<fetch::oef::pb::SearchQuery>(query);
          auto ds        = new_query->mutable_directed_search();

          for (const auto &loc : loc_res->GetLeaves())
          {
            if (loc->GetQueryFieldType() == "location")
            {
              FETCH_LOG_INFO(LOGGING_NAME,
                             "Setting location in query header from location constraint..");
              ds->mutable_target()->mutable_geo()->CopyFrom(loc->GetQueryFieldValue().l());
              break;
            }
          }

          ds->mutable_distance()->set_geo(0.0);

          auto sp = this_wp.lock();
          if (sp)
          {
            sp->HandleBroadcast(new_query, result);
          }
          else
          {
            FETCH_LOG_ERROR(LOGGING_NAME, "broadcast: lambda1: No shared pointer to DapManager");
          }
        }
        else
        {
          FETCH_LOG_INFO(LOGGING_NAME,
                         "No location constraint found in query! Skipping broadcast..");
          auto idseq = std::make_shared<IdentifierSequence>();
          result->set(idseq);
        }
        v.reset();
      });
    }
    else
    {
      auto new_query = std::make_shared<fetch::oef::pb::SearchQuery>(query);
      HandleBroadcast(new_query, result);
    }

    return result;
  }

  std::shared_ptr<Future<bool>> VisitQueryTreeLocal(std::shared_ptr<Branch> root)
  {
    auto sp = shared_from_this();

    auto result = std::make_shared<Future<bool>>();

    auto v = std::make_shared<PopulateFieldInformationVisitor>(dap_store_);
    v->SubmitVisitTask(root);
    v->MakeNotification().Then([root, sp, result]() {
      auto v = std::make_shared<CollectDapsVisitor>();
      v->SubmitVisitTask(root);
      v->MakeNotification().Then([root, sp, result]() {
        auto v = std::make_shared<AddMoreDapsBasedOnOptionsVisitor>(sp->dap_store_);
        v->SubmitVisitTask(root);
        v->MakeNotification().Then([result]() { result->set(true); });
      });
    });

    return result;
  }

protected:
  std::shared_ptr<Future<bool>> VisitQueryTreeNetwork(std::shared_ptr<Branch> root)
  {
    auto result = std::make_shared<Future<bool>>();
    auto sp     = shared_from_this();

    auto v = std::make_shared<PopulateActionsVisitorDescentPass>(sp, dap_store_);
    v->SubmitVisitTask(root);
    v->MakeNotification().Then([result]() { result->set(true); });

    return result;
  }

  void PlaneDistanceCheck(const std::string &                               plane,
                          const fetch::oef::pb::SearchQuery_DirectedSearch &header,
                          std::shared_ptr<Future<bool>> &                   future)
  {
    std::weak_ptr<Future<bool>> future_wp = future;

    bool resp = PlaneDistanceLookup(
        plane, header,
        [future_wp](const double &source_distance, const double &distance) {
          auto sp = future_wp.lock();
          if (sp)
          {
            if (source_distance <= distance)
            {
              sp->set(true);
            }
            else
            {
              FETCH_LOG_INFO(LOGGING_NAME, "Query will be ignored, because node distance (",
                             distance, ") is greater then source distance (", source_distance, ")");
              sp->set(false);
            }
          }
          else
          {
            FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to Future");
          }
        },
        [future_wp]() {
          auto sp = future_wp.lock();
          if (sp)
          {
            sp->set(false);
          }
          else
          {
            FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to Future");
          }
        });
    if (!resp)
    {
      future->set(true);
    }
  }

  bool PlaneDistanceLookup(const std::string &                                 plane,
                           const fetch::oef::pb::SearchQuery_DirectedSearch &  header,
                           std::function<void(const double &, const double &)> successHandler,
                           std::function<void()>                               errorHandler)
  {
    auto plane_desc = dap_store_->GetPlaneDescription(plane);
    if (plane_desc == nullptr)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Distance check is not possible, because there isn't a ", plane,
                     " dap with plane field!");
      return false;
    }
    auto request = std::make_shared<ConstructQueryConstraintObjectRequest>();
    request->set_operator_("DISTANCE");
    request->set_target_table_name(plane_desc->first);
    request->set_target_field_name(plane_desc->second.name());
    request->set_target_field_type(plane_desc->second.type());
    auto value = request->mutable_query_field_value();

    std::string dap_name{""};
    double      distance = 1e9;

    if (plane == "geo")
    {
      request->set_query_field_type(plane_desc->second.type());
      value->set_typecode("location");
      value->mutable_l()->CopyFrom(header.target().geo());
      dap_name = dap_store_->GetGeoDap();
      distance = header.distance().geo();
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Plane ", plane, " not yet supported by PlaneDistanceCheck!");
      return false;
    }
    FETCH_LOG_INFO(LOGGING_NAME, "Send distance query to DAP(", dap_name,
                   "): ", request->DebugString());

    using IN_PROTO  = ConstructQueryConstraintObjectRequest;
    using OUT_PROTO = ConstructQueryConstraintObjectRequest;

    auto convTask = std::make_shared<DapConversationTask<IN_PROTO, OUT_PROTO>>(
        dap_name, "calculate", ++single_call_msg_id, request, outbounds_);

    convTask->submit();

    convTask->messageHandler = [distance, successHandler,
                                errorHandler](std::shared_ptr<OUT_PROTO> response) {
      if (response->operator_() == "DISTANCE" &&
          response->query_field_value().typecode() == "double")
      {
        successHandler(distance, response->query_field_value().d());
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Got unexpected response for distance calculation call: ",
                       response->DebugString());
        errorHandler();
      }
    };

    convTask->errorHandler = [errorHandler](const std::string &dap_name, const std::string &path,
                                            const std::string &msg) {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to call ", dap_name, " with path: ", path, ": ", msg);
      errorHandler();
    };
    return true;
  }

  void DoBroadcast(std::shared_ptr<FutureComplexType<std::shared_ptr<IdentifierSequence>>> &future,
                   std::shared_ptr<fetch::oef::pb::SearchQuery>                             query)
  {
    auto convTask = std::make_shared<
        DapParallelConversationTask<fetch::oef::pb::SearchQuery, IdentifierSequence>>(
        ++parallel_call_msg_id, outbounds_, "");
    FETCH_LOG_INFO(LOGGING_NAME, "Start broadcasting to peers...");
    search_peer_store_->ForAllPeer([convTask, this, query](const std::string &peer) {
      FETCH_LOG_INFO(LOGGING_NAME, " Broadcast to search-peer: ", peer);
      convTask->Add(DapInputDataType<fetch::oef::pb::SearchQuery>{peer, "search", query});
      ++parallel_call_msg_id;
    });
    FETCH_LOG_INFO(LOGGING_NAME, "Submit broadcast tasks..");
    convTask->submit();
    convTask->MakeNotification().Then([future, convTask]() {
      auto idseq = std::make_shared<IdentifierSequence>();
      FETCH_LOG_INFO(LOGGING_NAME, "Broadcast done");
      std::size_t idx = 0;
      for (const auto &res : convTask->GetOutputs())
      {
        if (res->status().success())
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Search-node ", convTask->GetDapName(idx), " returned ",
                         res->identifiers_size(), " results!");
          for (const auto &id : res->identifiers())
          {
            idseq->add_identifiers()->CopyFrom(id);
          }
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Search-node ", convTask->GetDapName(idx),
                         " returned error message (", res->status().errorcode(),
                         ") when calling search:");
          for (const auto &m : res->status().narrative())
          {
            FETCH_LOG_WARN(LOGGING_NAME, "--> ", m);
          }
        }
        ++idx;
      }
      future->set(std::move(idseq));
    });
  }

  void HandleBroadcast(
      std::shared_ptr<fetch::oef::pb::SearchQuery>                            query,
      std::shared_ptr<FutureComplexType<std::shared_ptr<IdentifierSequence>>> future)
  {
    auto                      this_sp = this->shared_from_this();
    std::weak_ptr<DapManager> this_wp = this_sp;
    FETCH_LOG_INFO(LOGGING_NAME, "Handle broadcast, distance calculation started...");

    auto res = PlaneDistanceLookup(
        "geo", query->directed_search(),
        [query, this_wp, future](const double &/*source_distance*/, const double &distance) mutable {
          query->mutable_directed_search()->mutable_distance()->set_geo(distance);
          auto sp = this_wp.lock();
          if (sp)
          {
            sp->DoBroadcast(future, query);
          }
          else
          {
            FETCH_LOG_ERROR(LOGGING_NAME,
                            "HandleBroadcast: lambda1: No shared pointer to DapManager");
            auto idseq = std::make_shared<IdentifierSequence>();
            future->set(std::move(idseq));
          }
        },
        [future]() {
          FETCH_LOG_WARN(LOGGING_NAME,
                         "HandleBroadcast: lambda2: Failed to get distance. Skipping broadcast...");
          auto idseq = std::make_shared<IdentifierSequence>();
          future->set(std::move(idseq));
        });
    if (!res)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Stop broadcast, distance calculation failed..");
      auto idseq = std::make_shared<IdentifierSequence>();
      future->set(std::move(idseq));
    }
  }

protected:
  std::shared_ptr<DapStore>              dap_store_;
  std::shared_ptr<SearchPeerStore>       search_peer_store_;
  std::shared_ptr<OutboundConversations> outbounds_;
  std::shared_ptr<IdCache>               query_id_cache_;
  std::size_t                            parallel_call_msg_id = 2220;
  std::size_t                            single_call_msg_id   = 66600;
  std::size_t                            serial_call_msg_id   = 999000;

private:
  DapManager(const DapManager &other) = delete;              // { copy(other); }
  DapManager &operator=(const DapManager &other)  = delete;  // { copy(other); return *this; }
  bool        operator==(const DapManager &other) = delete;  // const { return compare(other)==0; }
  bool        operator<(const DapManager &other)  = delete;  // const { return compare(other)==-1; }
};
