#ifndef HTTP_OEF_INTERFACE_H
#define HTTP_OEF_INTERFACE_H

#include<iostream>
#include<fstream>
#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"
#include"commandline/parameter_parser.hpp"
#include"random/lfg.hpp"
#include"mutex.hpp"
#include"oef/NodeOEF.h"
#include"oef/schemaToJSON.h"
#include"oef/JSONToSchema.h"

#include<map>
#include<vector>
#include<set>
#include<algorithm>
#include<sstream>
using namespace fetch;
using namespace fetch::http;
using namespace fetch::commandline;


struct Transaction
{
  int64_t amount;
  byte_array::ByteArray fromAddress;
  byte_array::ByteArray notes;
  uint64_t time;
  byte_array::ByteArray toAddress;
  byte_array::ByteArray json;
};

struct Account
{
  int64_t balance = 0;
  std::vector< Transaction > history;
};


class HttpOEF : public fetch::http::HTTPModule
{

public:
  HttpOEF(NodeOEF *node) : node_{node}
  {

    HTTPModule::Post("/check", [this](ViewParameters const &params, HTTPRequest const &req) {
          return this->CheckUser(params, req);
        });
    HTTPModule::Post("/register", [this](ViewParameters const &params, HTTPRequest const &req) {
          return this->RegisterUser(params, req);
        });
    HTTPModule::Post("/balance", [this](ViewParameters const &params, HTTPRequest const &req) {
          return this->GetBalance(params, req);
        });
    HTTPModule::Post("/send", [this](ViewParameters const &params, HTTPRequest const &req) {
          return this->SendTransaction(params, req);
        });
    HTTPModule::Post("/get-transactions", [this](ViewParameters const &params, HTTPRequest const &req) {
        return this->GetHistory(params, req);
      });

    HTTPModule::Post("/register-instance", [this](ViewParameters const &params, HTTPRequest const &req) {
        return this->RegisterInstance(params, req);
      });

    HTTPModule::Post("/query-instance", [this](ViewParameters const &params, HTTPRequest const &req) {
        return this->QueryInstance(params, req);
      });

    HTTPModule::Post("/test", [this](ViewParameters const &params, HTTPRequest const &req) {
        return this->Test(params, req);
      });
  }

  ~HttpOEF() { delete node_;}

  HTTPResponse CheckUser(ViewParameters const &params, HTTPRequest const &req) {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );

    json::JSONDocument doc;
    try {
      doc = req.JSON();

      std::cout << "success" << std::endl;
      std::cout << req.body() << std::endl;
    } catch(...) {
      std::cout << "fail" << std::endl;
      std::cout << req.body() << std::endl;

      return HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    if(users_.find( doc["address"].as_byte_array() ) == users_.end())
      return HTTPResponse("{\"response\": \"false\"}");
    return HTTPResponse("{\"response\": \"true\"}");
  }

  HTTPResponse RegisterUser(ViewParameters const &params, HTTPRequest const &req) {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );

    json::JSONDocument doc;
    try {
      doc = req.JSON();
    } catch(...) {
      std::cout << req.body() << std::endl;

      return HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    if(users_.find( doc["address"].as_byte_array() ) != users_.end())
      return HTTPResponse("{\"response\": \"false\"}");

    users_.insert( doc["address"].as_byte_array() );
    accounts_[ doc["address"].as_byte_array()  ].balance = 300 + (lfg_() % 9700);

    return HTTPResponse("{}");
  }

  HTTPResponse GetBalance(ViewParameters const &params, HTTPRequest const &req) {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );

    json::JSONDocument doc;
    try {
      doc = req.JSON();
    } catch(...) {
      std::cout << req.body() << std::endl;

      return HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    script::Variant result = script::Variant::Object();

    if(users_.find( doc["address"].as_byte_array() ) == users_.end())
      return HTTPResponse("{\"balance\": 0}");

    result["response"] = accounts_[ doc["address"].as_byte_array() ].balance;

    std::stringstream ret;
    ret << result;
    return HTTPResponse(ret.str());
  }

  HTTPResponse SendTransaction(ViewParameters const &params, HTTPRequest const &req) {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );

    json::JSONDocument doc;
    try {
      doc = req.JSON();
    } catch(...) {
      std::cout << req.body() << std::endl;

      return HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    Transaction tx;

    tx.fromAddress = doc["fromAddress"].as_byte_array();
    tx.amount = doc["balance"].as_int();
    tx.notes = doc["notes"].as_byte_array();
    tx.time = doc["time"].as_int();
    tx.toAddress = doc["toAddress"].as_byte_array();
    tx.json = req.body();

    if(users_.find( tx.fromAddress ) == users_.end())
      return HTTPResponse("{\"response\": \"false\", \"reason\": \"fromAddress does not exist\"}");

    if(users_.find( tx.toAddress ) == users_.end())
      return HTTPResponse("{\"response\": \"false\", \"reason\": \"toAddress does not exist\"}");


    if(accounts_.find(tx.fromAddress) == accounts_.end())
    {
      accounts_[tx.fromAddress].balance = 0;
    }

    if(accounts_[tx.fromAddress].balance < tx.amount)
    {
      return HTTPResponse("{\"response\": \"false\", \"reason\": \"insufficient funds\"}");
    }

    accounts_[tx.fromAddress].balance -= tx.amount;
    accounts_[tx.toAddress].balance += tx.amount;

    accounts_[tx.fromAddress].history.push_back(tx);
    accounts_[tx.toAddress].history.push_back(tx);

    script::Variant result = script::Variant::Object();
    result["response"] = accounts_[tx.fromAddress].balance;

    std::stringstream ret;
    ret << result;
    return HTTPResponse(ret.str());
  }

  HTTPResponse GetHistory(ViewParameters const &params, HTTPRequest const &req) {
    std::lock_guard< fetch::mutex::Mutex > lock( mutex_ );
    json::JSONDocument doc;
    try {
      doc = req.JSON();
    } catch(...) {
      std::cout << req.body() << std::endl;

      return HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    auto address =  doc["address"].as_byte_array();
    if(users_.find( address ) == users_.end())
      return HTTPResponse("{\"response\": \"false\", \"reason\": \"toAddress does not exist\"}");


    auto &account = accounts_[address];

    std::size_t n = std::min(20, int(account.history.size()) );

    script::Variant result = script::Variant::Object();
    script::Variant history = script::Variant::Array(n);

    for(std::size_t i=0; i < n; ++i)
    {
      history[i] = account.history[ account.history.size() - 1 - i].json;
    }

    result["data"] = history;
    result["response"] = "yes";

    std::stringstream ret;
    ret << result;
    return HTTPResponse(ret.str());
  }

  HTTPResponse RegisterInstance(ViewParameters const &params, HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();

      std::string id = doc["ID"].as_byte_array();
      Instance instance(doc);
      auto success = node_->RegisterInstance(id, instance);

      return HTTPResponse("{\"response\": \""+success+"\"}");
    } catch (...) {
      return HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  HTTPResponse QueryInstance(ViewParameters const &params, HTTPRequest const &req) {

    json::JSONDocument doc;

    std::cout << "Hitting query" << std::endl;

    try {
      doc = req.JSON();

      std::cout << "const query" << std::endl;
      QueryModel query(doc);

      std::cout << "ar query" << std::endl;

      auto agents = node_->Query(query);

      std::cout << "thihg query" << std::endl;

      // Build the response here, TODO: (`HUT`) : make this cleaner
      
      std::string response{"{ \"agents\": [ "};

      for (auto i = agents.begin(); i != agents.end();) {
        response += std::string("\"") + *i + std::string("\"");

        if(++i != agents.end()){
          response += ",";
        }
      }

      if(agents.size() == 0) {
          response += "\"none\"";
      }

      response += "] }";

      std::cout << response << std::endl;

      return HTTPResponse("{\"response\": "+response+"}");
    } catch (...) {
      return HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  HTTPResponse Test(ViewParameters const &params, HTTPRequest const &req) {

    node_->test();
    return HTTPResponse("{\"response\": \"success\"}");
  }

public:
  std::vector< Transaction >                             transactions_;
  std::map< fetch::byte_array::BasicByteArray, Account > accounts_;
  std::set< fetch::byte_array::BasicByteArray >          users_;
  fetch::random::LaggedFibonacciGenerator<>              lfg_;
  fetch::mutex::Mutex                                    mutex_;

private:
  NodeOEF *node_;
};

#endif
