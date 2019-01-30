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

#include "core/logger.hpp"
#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"
#include "http/server.hpp"
#include "network_classes.hpp"
#include "variant/variant.hpp"

namespace fetch {
namespace network_benchmark {

template <typename T>
class HttpInterface : public fetch::http::HTTPModule
{
public:
  explicit HttpInterface(std::shared_ptr<T> node)
    : node_{node}
  {
    AttachPages();
  }

  void AttachPages()
  {
    LOG_STACK_TRACE_POINT;
    HTTPModule::Post("/add-endpoint",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->AddEndpoint(params, req);
                     });

    HTTPModule::Post("/transactions",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->Transactions(params, req);
                     });

    HTTPModule::Post("/transactions-per-call",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->SetTPC(params, req);
                     });

    HTTPModule::Post("/reset",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->Reset(params, req);
                     });

    HTTPModule::Post("/transactions-hash",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->TransactionsHash(params, req);
                     });

    HTTPModule::Post("/transactions-to-sync",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->TransactionsToSync(params, req);
                     });

    HTTPModule::Post("/stop-condition",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->StopCondition(params, req);
                     });

    HTTPModule::Post("/is-slave",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->IsSlave(params, req);
                     });

    HTTPModule::Post("/start-time",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->StartTime(params, req);
                     });

    HTTPModule::Post("/start-test-as-master",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->StartTestAsMaster(params, req);
                     });

    HTTPModule::Post("/time-to-complete",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->TimeToComplete(params, req);
                     });

    HTTPModule::Post("/finished",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->Finished(params, req);
                     });

    HTTPModule::Post("/transaction-size",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->TransactionSize(params, req);
                     });
  }

  HttpInterface(HttpInterface &&rhs)
  {
    LOG_STACK_TRACE_POINT;
    node_ = std::move(rhs.node());
    AttachPages();
  }

  http::HTTPResponse AddEndpoint(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);

    LOG_STACK_TRACE_POINT;
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      network_benchmark::Endpoint endpoint(doc);

      node_->AddEndpoint(endpoint);

      return http::HTTPResponse(successString);
    }
    catch (...)
    {
      return http::HTTPResponse(failureString);
    }
  }

  http::HTTPResponse Transactions(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    auto transactions = node_->GetTransactions();

    variant::Variant result = variant::Variant::Array(transactions.size());

    std::size_t index = 0;
    for (auto &i : transactions)
    {
      result[index++] = byte_array::ToBase64(i.summary().transaction_hash);
    }

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse SetTPC(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);

    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      uint32_t tpc = doc["transactions"].As<uint32_t>();

      node_->transactionsPerCall(tpc);

      return http::HTTPResponse(successString);
    }
    catch (...)
    {
      return http::HTTPResponse(failureString);
    }
  }

  http::HTTPResponse Reset(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    node_->Reset();
    return http::HTTPResponse(successString);
  }

  http::HTTPResponse TransactionsHash(http::ViewParameters const &params,
                                      http::HTTPRequest const &   req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    auto res = node_->TransactionsHash();

    variant::Variant result = variant::Variant::Object();

    result["numberOfTransactions"] = res.first;
    result["hash"]                 = res.second;

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse TransactionsToSync(http::ViewParameters const &params,
                                        http::HTTPRequest const &   req)
  {
    FETCH_UNUSED(params);

    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      node_->TransactionsToSync(doc["transactionsToSync"].As<uint64_t>());

      return http::HTTPResponse(successString);
    }
    catch (...)
    {
      return http::HTTPResponse(failureString);
    }
  }

  http::HTTPResponse StopCondition(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);

    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      node_->stopCondition(doc["stopCondition"].As<uint64_t>());

      return http::HTTPResponse(successString);
    }
    catch (...)
    {
      return http::HTTPResponse(failureString);
    }
  }

  http::HTTPResponse IsSlave(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    node_->isSlave();
    return http::HTTPResponse(successString);
  }

  http::HTTPResponse StartTime(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);

    LOG_STACK_TRACE_POINT;
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      node_->StartTime(doc["startTime"].As<uint64_t>());

      return http::HTTPResponse(successString);
    }
    catch (...)
    {
      return http::HTTPResponse(failureString);
    }
  }

  http::HTTPResponse StartTestAsMaster(http::ViewParameters const &params,
                                       http::HTTPRequest const &   req)
  {
    FETCH_UNUSED(params);

    LOG_STACK_TRACE_POINT;
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      node_->StartTestAsMaster(doc["startTime"].As<uint64_t>());

      return http::HTTPResponse(successString);
    }
    catch (...)
    {
      return http::HTTPResponse(failureString);
    }
  }

  http::HTTPResponse TimeToComplete(http::ViewParameters const &params,
                                    http::HTTPRequest const &   req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    LOG_STACK_TRACE_POINT;
    variant::Variant result = variant::Variant::Object();

    result["timeToComplete"] = node_->TimeToComplete();

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse Finished(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    FETCH_UNUSED(params);
    FETCH_UNUSED(req);

    LOG_STACK_TRACE_POINT;
    variant::Variant result = variant::Variant::Object();

    result["finished"] = node_->finished();

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse TransactionSize(http::ViewParameters const &params,
                                     http::HTTPRequest const &   req)
  {
    FETCH_UNUSED(params);

    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      node_->TransactionSize(doc["transactionSize"].As<uint32_t>());

      return http::HTTPResponse(successString);
    }
    catch (...)
    {
      return http::HTTPResponse(failureString);
    }
  }

  const std::shared_ptr<T> &node() const
  {
    return node_;
  };

private:
  std::shared_ptr<T> node_;
  const std::string  successString{"{\"response\": \"success\" }"};
  const std::string  failureString{
      "{\"response\": \"failure\", \"reason\": \"problems with parsing "
      "JSON!\"}"};
};

}  // namespace network_benchmark
}  // namespace fetch
