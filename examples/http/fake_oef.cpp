#include<iostream>
#include<fstream>
#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"
#include"commandline/parameter_parser.hpp"

#include<map>
#include<vector>
#include<sstream>
using namespace fetch::http;
using namespace fetch::commandline;

/*
checkUser: {
    address = "830A0B9D-73EE-4001-A413-72CFCD8E91F3";
    post = check;
}

registerUser: {
    address = "830A0B9D-73EE-4001-A413-72CFCD8E91F3";
    amount = 183739910101;
    post = register;
}

Get BalanceX: {
    address = "830A0B9D-73EE-4001-A413-72CFCD8E91F3";
    post = balance;
}

Make Transaction: {
    balance = 200;
    event = SEND;
    fromAddress = "830A0B9D-73EE-4001-A413-72CFCD8E91F3";
    notes = TEST;
    post = transaction;
    time = 1519650052994;
    toAddress = "6164D5A6-A26E-43E4-BA96-A1A8787091A0";
}

getTransactions: {
    address = "830A0B9D-73EE-4001-A413-72CFCD8E91F3";
    post = "transaction_history";
}
*/
class FakeOEF : public fetch::http::HTTPModule 
{

public:
  FakeOEF() 
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
  }

  HTTPResponse CheckUser(ViewParameters const &params, HTTPRequest const &req) {
    return HTTPResponse("not implemented");    
  }

  HTTPResponse RegisterUser(ViewParameters const &params, HTTPRequest const &req) {
    return HTTPResponse("not implemented");    
  }

  HTTPResponse GetBalance(ViewParameters const &params, HTTPRequest const &req) {
    return HTTPResponse("not implemented");    
  }

  HTTPResponse SendTransaction(ViewParameters const &params, HTTPRequest const &req) {
    return HTTPResponse("not implemented");    
  }

  HTTPResponse GetHistory(ViewParameters const &params, HTTPRequest const &req) {
    return HTTPResponse("not implemented");    
  }
  
  
  
public:
  std::vector< fetch::byte_array::BasicByteArray > transactions_;
  std::map< fetch::byte_array::BasicByteArray, int > accounts_;    
};


int main(int argc, char const **argv) 
{

  ParamsParser params;
  params.Parse(argc, argv);

  fetch::network::ThreadManager tm(8);  
  HTTPServer http_server(8080, &tm);
  FakeOEF oef_http_interface;  
  
  http_server.AddMiddleware( fetch::http::middleware::AllowOrigin("*") );       
  http_server.AddMiddleware( fetch::http::middleware::ColorLog);
  http_server.AddModule(oef_http_interface);

  tm.Start();
  
  std::cout << "Ctrl-C to stop" << std::endl;
  while(true) 
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
  }

  tm.Stop();
  
  return 0;
}
