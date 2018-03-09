#ifndef HTTP_OEF_INTERFACE_HPP
#define HTTP_OEF_INTERFACE_HPP

// This file defines the HTTP interface that allows interaction with the (fake) ledger and the OEF. It will need to be split into two at some point to handle these
// components seperately

#include<iostream>
#include<fstream>
#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"
#include"commandline/parameter_parser.hpp"
#include"random/lfg.hpp"
#include"mutex.hpp"
#include"script/variant.hpp"
#include"oef/oef.hpp"

#include<map>
#include<vector>
#include<algorithm> // TODO: (`HUT`) : remove unnecc. includes
#include<sstream>

namespace fetch
{
namespace http_oef
{

class HttpOEF : public fetch::http::HTTPModule
{
public:
  // In constructor attach the callbacks for the http pages we want
  HttpOEF(std::shared_ptr<oef::NodeOEF> oef) : oef_{oef} {
    // Ledger functionality
    HTTPModule::Post("/check", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
          return this->CheckUser(params, req);
        });
    HTTPModule::Post("/register", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
          return this->RegisterUser(params, req);
        });
    HTTPModule::Post("/balance", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
          return this->GetBalance(params, req);
        });
    HTTPModule::Post("/send", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
          return this->SendTransaction(params, req);
        });
    HTTPModule::Post("/get-transactions", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->GetHistory(params, req);
      });

    // OEF functionality
    HTTPModule::Post("/register-instance", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->RegisterInstance(params, req);
      });

    HTTPModule::Post("/query-instance", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->QueryInstance(params, req);
      });

    // OEF debug functions
    HTTPModule::Post("/echo-query", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->EchoQuery(params, req);
      });

    HTTPModule::Post("/echo-instance", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->EchoInstance(params, req);
      });

    HTTPModule::Post("/ping-aeas", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->PingAEAs();
      });

    HTTPModule::Post("/buy-from-aea", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->BuyFromAEA(params, req);
      });

    HTTPModule::Post("/test", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->Test(params, req);
      });
  }

  // Check that a user exists in our ledger
  http::HTTPResponse CheckUser(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();

      std::cout << "correctly parsed JSON: " << req.body() << std::endl;
    } catch(...) {
      std::cout << req.body() << std::endl;

      return http::HTTPResponse("{\"response\": \"fail\", \"reason\": \"problems with parsing JSON\"}");
    }

    if(!(oef_->IsLedgerUser(doc["address"].as_byte_array()))) {
      return http::HTTPResponse("{\"response\": \"success\", \"value\": \"false\"}");
    }

    return http::HTTPResponse("{\"response\": \"success\", \"value\": \"true\"}");
  }

  // Create a new account on the system, initialise it with random balance
  http::HTTPResponse RegisterUser(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;
    } catch(...) {
      std::cout << req.body() << std::endl;

      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    if(!(oef_->AddLedgerUser(doc["address"].as_byte_array()))) {
      return http::HTTPResponse("{\"response\": \"fail\"}");
    }

    return http::HTTPResponse("{\"response\": \"success\"}");
  }

  // Get balance of user, note if the user doesn't exist this returns 0
  http::HTTPResponse GetBalance(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;
    } catch(...) {
      std::cout << req.body() << std::endl;

      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    script::Variant result = script::Variant::Object();

    if(!(oef_->IsLedgerUser(doc["address"].as_byte_array()))) {
      return http::HTTPResponse("{\"response\": 0}");
    }

    result["value"]    = oef_->GetUserBalance(doc["address"].as_byte_array());
    result["response"] = "success";

    std::stringstream ret;
    ret << result;
    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse SendTransaction(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON for tran: " << req.body() << std::endl;
    } catch(...) {
      std::cout << req.body() << std::endl;

      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    auto result = oef_->SendTransaction(doc);

    std::stringstream ret;
    ret << result;
    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse GetHistory(http::ViewParameters const &params, http::HTTPRequest const &req) {
    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;
    } catch(...) {
      std::cout << req.body() << std::endl;

      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    auto result = oef_->GetHistory(doc["address"].as_byte_array());

    std::stringstream ret;
    ret << result;
    return http::HTTPResponse(ret.str());
  }

  // OEF functions
  http::HTTPResponse RegisterInstance(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();

      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      std::string id = doc["ID"].as_byte_array();
      schema::Instance instance(doc["instance"]);
      auto ret = oef_->RegisterInstance(id, instance);

      return http::HTTPResponse("{\"response\": \"success\", \"value\": \""+ret+"\"}");
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"fail\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  http::HTTPResponse QueryInstance(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      schema::QueryModel query(doc);

      auto agents = oef_->Query(query);

      script::Variant response       = script::Variant::Object();
      response["response"]           = script::Variant::Object();
      response["response"]["agents"] = script::Variant::Array(agents.size());

      for (int i = 0; i < agents.size(); ++i) {
        response["response"]["agents"][i] = script::Variant(agents[i]);
      }

      std::ostringstream ret;
      ret << response;

      return http::HTTPResponse(ret.str());
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  // Functions to test JSON serialisation/deserialisation
  http::HTTPResponse EchoQuery(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      schema::QueryModel query(doc);

      std::ostringstream ret;
      ret << query.variant();

      return http::HTTPResponse(ret.str());
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  http::HTTPResponse EchoInstance(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      schema::Instance instance(doc["instance"]);

      std::ostringstream ret;
      ret << instance.variant();

    return http::HTTPResponse(ret.str());
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  http::HTTPResponse PingAEAs() {
		std::cerr << "doing the thing" << std::endl;
		oef_->PingAllAEAs();
		std::cerr << "done the thing" << std::endl;
    return http::HTTPResponse("{\"response\": \"success\"}");
  }

  http::HTTPResponse Test(http::ViewParameters const &params, http::HTTPRequest const &req) {
    return http::HTTPResponse("{\"response\": \"success\"}");
  }

  http::HTTPResponse BuyFromAEA(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      auto result = oef_->BuyFromAEA(doc["ID"].as_byte_array());

      std::ostringstream ret;
      ret << result;

    return http::HTTPResponse(ret.str());
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

private:
  std::shared_ptr<oef::NodeOEF> oef_;
};

}
}
#endif
