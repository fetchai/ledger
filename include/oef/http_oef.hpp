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
    HTTPModule::Post("/query", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->Query(params, req);
      });

    HTTPModule::Post("/multi-query", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->MultiQuery(params, req);
      });

    HTTPModule::Post("/register-instance", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->RegisterInstance(params, req);
      });

    HTTPModule::Post("/deregister-instance", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->DeregisterInstance(params, req);
      });

    HTTPModule::Post("/query-for-agents", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->QueryForAgents(params, req);
      });

    HTTPModule::Post("/query-for-agents-instances", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->QueryForAgentsInstances(params, req);
      });

    HTTPModule::Post("/service-directory", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->ServiceDirectory();
      });

    // UI functionality
    HTTPModule::Post("/ui-query", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->UiQuery(params, req);
      });

    HTTPModule::Post("/add-connection", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->AddConnection(params, req);
      });

    // OEF debug functions
    HTTPModule::Post("/echo-query", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->EchoQuery(params, req);
      });

    HTTPModule::Post("/echo-multi-query", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->EchoMultiQuery(params, req);
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

    // Debug functionality
    HTTPModule::Post("/debug-all-nodes", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->DebugAllNodes();
      });

    HTTPModule::Post("/debug-all-endpoints", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->DebugAllEndpoints();
      });

    HTTPModule::Post("/debug-connections", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->DebugConnections();
      });

    HTTPModule::Post("/debug-endpoint", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->DebugEndpoint();
      });

    HTTPModule::Post("/debug-all-agents", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->DebugAllAgents();
      });

    HTTPModule::Post("/debug-all-events", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->DebugAllEvents(params, req);
      });

    HTTPModule::Post("/debug-all-history", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->DebugEndpoint();
      });

    HTTPModule::Post("/set-node-latlong", [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
        return this->SetNodeLatLong(params, req);
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
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;
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

      schema::Instance instance(doc["instance"]);
      std::string id = instance.values()["name"];

      auto ret = oef_->RegisterInstance(id, instance);

      return http::HTTPResponse("{\"response\": \"success\", \"value\": \""+ret+"\"}");
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"fail\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  http::HTTPResponse DeregisterInstance(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();

      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      std::string id = doc["ID"].as_byte_array();
      schema::Instance instance(doc["instance"]);
      oef_->DeregisterInstance(id, instance);

      return http::HTTPResponse("{\"response\": \"success\"}");
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"fail\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  http::HTTPResponse Query(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      schema::QueryModel query(doc);

      auto agents = oef_->Query("HTTP_interface", query);

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
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON!\"}"); // TODO: (`HUT`) : standardise response
    }
  }

  http::HTTPResponse MultiQuery(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      // split here
      json::JSONDocument doc1 = doc["aeaQuery"];
      json::JSONDocument doc2 = doc["forwardingQuery"]; // TODO: (`HUT`) : don't copy this

      schema::QueryModel aeaQuery(doc1);
      schema::QueryModel forwardingQuery(doc2);

      schema::QueryModelMulti multiQ(aeaQuery, forwardingQuery);

      auto agents = oef_->AEAQueryMulti(doc["ID"].as_byte_array(), multiQ);

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
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON!\"}"); // TODO: (`HUT`) : standardise response
    }
  }

  http::HTTPResponse QueryForAgents(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      schema::QueryModel query(doc);

      auto agents = oef_->Query("HTTP_interface", query);

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

  http::HTTPResponse ServiceDirectory() {

    auto serviceDirectory = oef_->ServiceDirectory();

    std::ostringstream ret;
    ret << serviceDirectory;

    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse QueryForAgentsInstances(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      schema::QueryModel query(doc);

      auto agentsInstances = oef_->QueryAgentsInstances(query);

      script::Variant response       = script::Variant::Object();
      response["response"]           = script::Variant::Array(agentsInstances.size());

      // Build a response
      for(int i = 0;i < agentsInstances.size();i++) {
        response["response"][i]    = script::Variant::Array(2);
        response["response"][i][0] = agentsInstances[i].first.variant();
        response["response"][i][1] = script::Variant(agentsInstances[i].second);
      }

      std::ostringstream ret;
      ret << response;

      return http::HTTPResponse(ret.str());
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  // UI query
  http::HTTPResponse UiQuery(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      float angle1 = doc["angle1"].as_double();
      float angle2 = doc["angle2"].as_double();

      std::string name = doc["name"].as_byte_array();

      std::string searchText = doc["searchtext"].as_byte_array();

      std::istringstream buffer(searchText);
      int priceSearch;
      buffer >> priceSearch;

      schema::Attribute price   { "price",        schema::Type::Int, true}; // guarantee all DMs have this
      schema::ConstraintType customConstraint{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Lt, priceSearch}}};
      schema::Constraint price_c   { price    ,    customConstraint};

      // Query is build up from constraints
      schema::QueryModel query1{{price_c}};

      // create a special forwarding query 
      schema::Instance instance = oef_->getInstance();

      schema::QueryModel forwardingQuery{};

      forwardingQuery.lat()    = instance.values()["latitude"];
      forwardingQuery.lng()    = instance.values()["longitude"];
      forwardingQuery.angle1() = angle1;
      forwardingQuery.angle2() = angle2;

      schema::QueryModelMulti multiQ{query1, forwardingQuery};

      auto agents = oef_->AEAQueryMulti(name, multiQ);

      //return http::HTTPResponse(ret.str());
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }

    return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
  }

  http::HTTPResponse AddConnection(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      schema::Endpoint endpoint{doc};
      oef_->addConnection(endpoint);

      std::ostringstream ret;
      ret << endpoint.variant();
      std::cout << ret.str() << std::endl;

      return http::HTTPResponse("{\"response\": \"success\" }");
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

  http::HTTPResponse EchoMultiQuery(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      json::JSONDocument doc1 = doc["aeaQuery"];
      json::JSONDocument doc2 = doc["forwardingQuery"]; // TODO: (`HUT`) : don't copy this

      schema::QueryModel aeaQuery(doc1);
      schema::QueryModel forwardingQuery(doc2);

      schema::QueryModelMulti multiQ(aeaQuery, forwardingQuery);

      std::ostringstream ret;
      ret << multiQ.variant();

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

  // TODO: (`HUT`) : remove test functionality
  http::HTTPResponse PingAEAs() {
    oef_->PingAllAEAs();
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

      auto result = oef_->BuyFromAEA("http_interface", doc["ID"].as_byte_array());

      std::ostringstream ret;
      ret << result;

    return http::HTTPResponse(ret.str());
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"false\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

  // Debug functionality
  http::HTTPResponse DebugAllNodes() {

    auto result = oef_->DebugAllNodes();
    std::ostringstream ret;
    ret << result;
    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse DebugAllEndpoints() {

    auto result = oef_->DebugAllEndpoints();
    std::ostringstream ret;
    ret << result;
    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse DebugConnections() {

    auto result = oef_->DebugConnections();
    std::ostringstream ret;
    ret << result;
    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse DebugEndpoint() {

    auto result = oef_->DebugEndpoint();
    std::ostringstream ret;
    ret << result;
    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse DebugAllAgents() {

    auto result = oef_->DebugAllAgents();
    std::ostringstream ret;
    ret << result;
    return http::HTTPResponse(ret.str());
  }

  http::HTTPResponse DebugAllEvents(http::ViewParameters const &params, http::HTTPRequest const &req) {

    int defaultNumber = 3000;
    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON: " << req.body() << std::endl;

      // TODO: (`HUT`) : revert this hack once json doc parser fixed
      float maxNumber = doc["max_number"].is_undefined() ? defaultNumber : doc["max_number"].as_double(); // default 10 events

      std::cout << "debugging " << maxNumber << " events" << std::endl;
      auto result = oef_->DebugAllEvents(maxNumber);

      std::ostringstream ret;
      ret << result;

    return http::HTTPResponse(ret.str());
    } catch (...) {

      auto result = oef_->DebugAllEvents(defaultNumber);
      std::ostringstream ret;
      ret << result;
      return http::HTTPResponse(ret.str());
    }
  }

  http::HTTPResponse SetNodeLatLong(http::ViewParameters const &params, http::HTTPRequest const &req) {

    json::JSONDocument doc;
    try {
      doc = req.JSON();
      std::cout << "correctly parsed JSON:: " << req.body() << std::endl;

      // debug
      std::ostringstream ret;
      ret << req.body();
      std::string debug(ret.str());

      // TODO: (`HUT`) : this doesn't actually save you when submitting '{}'
      if(doc["latitude"].is_undefined() || doc["longitude"].is_undefined()) {
        throw 1;
      }

      float latitude  =  doc["latitude"].as_double();
      float longitude =  doc["longitude"].as_double();

      // reconfigure this node to have this latlong
      schema::Instance instance = oef_->getInstance();

      const std::unordered_map<std::string,std::string> &values = instance.values();
      std::unordered_map<std::string,std::string> replicatedValues;

      // TODO: (`HUT`) : refactor this.
      for(auto &i : values) {
        if(i.first.compare("latitude") == 0) {

          replicatedValues["latitude"] = std::to_string(latitude);
          continue;
        } else if(i.first.compare("longitude") == 0) {
          replicatedValues["longitude"] = std::to_string(longitude);
          continue;
        }

        replicatedValues[i.first] = i.second;
      }

      schema::Instance instNew{instance.model(), replicatedValues};

      oef_->setInstance(instNew);

      return http::HTTPResponse("{\"response\": \"success\"}");
    } catch (...) {
      return http::HTTPResponse("{\"response\": \"fail\", \"reason\": \"problems with parsing JSON\"}");
    }
  }

private:
  std::shared_ptr<oef::NodeOEF> oef_;
};

}
}
#endif
