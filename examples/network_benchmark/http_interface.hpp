#ifndef NETWORK_BENCHMARK_HTTP_INTERFACE_HPP
#define NETWORK_BENCHMARK_HTTP_INTERFACE_HPP

#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"

#include"script/variant.hpp"
#include"./network_classes.hpp"
#include"logger.hpp"

namespace fetch
{
namespace network_benchmark
{

template <typename T>
class HttpInterface : public fetch::http::HTTPModule {
public:

  explicit HttpInterface(std::shared_ptr<T> node) : node_{node}
  {
    AttachPages();
  }

  void AttachPages()
  {
    LOG_STACK_TRACE_POINT ;
    HTTPModule::Post("/add-endpoint",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->AddEndpoint(params, req); });

    HTTPModule::Post("/start",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->Start(params, req); });

    HTTPModule::Post("/stop",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->Stop(params, req); });

    HTTPModule::Post("/transactions",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->Transactions(params, req); });

    HTTPModule::Post("/set-rate",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->SetRate(params, req); });

    HTTPModule::Post("/set-transactions-per-call",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->SetTPC(params, req); });

    HTTPModule::Post("/reset",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->Reset(params, req); });

    HTTPModule::Post("/transactions-hash",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->TransactionsHash(params, req); });

    HTTPModule::Post("/transactions-to-sync",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->TransactionsToSync(params, req); });

    HTTPModule::Post("/stop-condition",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->StopCondition(params, req); });

    HTTPModule::Post("/start-time",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->StartTime(params, req); });

    HTTPModule::Post("/time-to-complete",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->TimeToComplete(params, req); });

    HTTPModule::Post("/finished",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->Finished(params, req); });

    HTTPModule::Post("/transaction-size",\
    [this](http::ViewParameters const &params, http::HTTPRequest const &req)\
    { return this->TransactionSize(params, req); });
  }

  HttpInterface(HttpInterface&& rhs)
  {
    LOG_STACK_TRACE_POINT ;
    node_ = std::move(rhs.node());
    AttachPages();
  }

  http::HTTPResponse AddEndpoint(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    LOG_STACK_TRACE_POINT ;
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      network_benchmark::Endpoint endpoint(doc);

      node_->AddEndpoint(endpoint);

      return http::HTTPResponse("{\"response\": \"success\" }");
    } catch (...)
    {
      return http::HTTPResponse("{\"response\": \"failure\", \"reason\": \"problems with parsing JSON!\"}");
    }
  }

  http::HTTPResponse Start(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    node_->Start();
    return http::HTTPResponse("{\"response\": \"success\" }");
  }

  http::HTTPResponse Stop(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    node_->Stop();
    return http::HTTPResponse("{\"response\": \"success\" }");
  }

  http::HTTPResponse Transactions(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    auto transactions = node_->GetTransactions();

    script::Variant result = script::Variant::Array(transactions.size());

    int index = 0;
    for (auto &i : transactions) {
      result[index++] = byte_array::ToHex(i.summary().transaction_hash);
    }

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse SetRate(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      int rate = doc["rate"].as_int();

      node_->setRate(rate);

      return http::HTTPResponse("{\"response\": \"success\" }");
    } catch (...)
    {
      return http::HTTPResponse("{\"response\": \"failure\", \"reason\": \"problems with parsing JSON!\"}");
    }
  }

  http::HTTPResponse SetTPC(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      int tpc = doc["transactions"].as_int();

      node_->SetTransactionsPerCall(tpc);

      return http::HTTPResponse("{\"response\": \"success\" }");
    } catch (...)
    {
      return http::HTTPResponse("{\"response\": \"failure\", \"reason\": \"problems with parsing JSON!\"}");
    }
  }

  http::HTTPResponse Reset(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    node_->Reset();
    return http::HTTPResponse("{\"response\": \"success\" }");
  }

  http::HTTPResponse TransactionsHash(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    auto res = node_->TransactionsHash();

    script::Variant result = script::Variant::Object();

    result["numberOfTransactions"] = res.first;
    result["hash"]                 = res.second;

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse TransactionsToSync(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      node_->SetTransactionsToSync(doc["transactionsToSync"].as_int());

      return http::HTTPResponse("{\"response\": \"success\" }");
    } catch (...)
    {
      return http::HTTPResponse("{\"response\": \"failure\", \"reason\": \"problems with parsing JSON!\"}");
    }
  }

  http::HTTPResponse StopCondition(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      node_->setStopCondition(doc["stopCondition"].as_int());

      return http::HTTPResponse("{\"response\": \"success\" }");
    } catch (...)
    {
      return http::HTTPResponse("{\"response\": \"failure\", \"reason\": \"problems with parsing JSON!\"}");
    }
  }

  http::HTTPResponse StartTime(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    LOG_STACK_TRACE_POINT ;
    json::JSONDocument doc;
    try
    {
      doc = req.JSON();
      std::cerr << "correctly parsed JSON: " << req.body() << std::endl;

      node_->setStartTime(doc["startTime"].as_int());

      return http::HTTPResponse("{\"response\": \"success\" }");
    } catch (...)
    {
      return http::HTTPResponse("{\"response\": \"failure\", \"reason\": \"problems with parsing JSON!\"}");
    }
  }

  http::HTTPResponse TimeToComplete(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    LOG_STACK_TRACE_POINT ;
    script::Variant result = script::Variant::Object();

    result["timeToComplete"] = node_->timeToComplete();

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse Finished(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    LOG_STACK_TRACE_POINT ;
    script::Variant result = script::Variant::Object();

    result["finished"] = node_->finished();

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse TransactionSize(http::ViewParameters const &params, http::HTTPRequest const &req)
  {
    LOG_STACK_TRACE_POINT ;
    script::Variant result = script::Variant::Object();

    result["transactionSize"] = node_->TransactionSize();

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }


  const std::shared_ptr<T> &node() const { return node_; };

private:
  std::shared_ptr<T> node_;
};

}
}
#endif
