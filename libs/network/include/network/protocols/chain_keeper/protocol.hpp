#ifndef PROTOCOLS_CHAIN_KEEPER_PROTOCOL_HPP
#define PROTOCOLS_CHAIN_KEEPER_PROTOCOL_HPP
#include "byte_array/decoders.hpp"
#include "crypto/fnv.hpp"
#include "http/module.hpp"
#include "protocols/chain_keeper/commands.hpp"
#include "protocols/chain_keeper/controller.hpp"
#include "service/function.hpp"

#include <unordered_set>

namespace fetch {
namespace protocols {

class ChainKeeperProtocol : public ChainKeeperController,
                            public fetch::service::Protocol,
                            public fetch::http::HTTPModule {
 public:
  typedef typename ChainKeeperController::transaction_summary_type
      transaction_summary_type;
  typedef typename ChainKeeperController::transaction_type transaction_type;

  typedef fetch::service::ServiceClient<fetch::network::TCPClient> client_type;
  typedef std::shared_ptr<client_type> client_shared_ptr_type;

  ChainKeeperProtocol(network::NetworkManager *network_manager,
                      uint64_t const &protocol, EntryPoint &details)
      : ChainKeeperController(protocol, network_manager, details),
        fetch::service::Protocol() {
    using namespace fetch::service;

    // RPC Protocol
    ChainKeeperController *controller = (ChainKeeperController *)this;
    Protocol::Expose(ChainKeeperRPC::PING, this, &ChainKeeperProtocol::Ping);
    Protocol::Expose(ChainKeeperRPC::HELLO, controller,
                     &ChainKeeperController::Hello);
    Protocol::Expose(ChainKeeperRPC::PUSH_TRANSACTION, controller,
                     &ChainKeeperController::PushTransaction);
    Protocol::Expose(ChainKeeperRPC::GET_TRANSACTIONS, controller,
                     &ChainKeeperController::GetTransactions);
    Protocol::Expose(ChainKeeperRPC::GET_SUMMARIES, controller,
                     &ChainKeeperController::GetSummaries);

    // TODO: Move to separate protocol
    Protocol::Expose(ChainKeeperRPC::LISTEN_TO, controller,
                     &ChainKeeperController::ListenTo);
    Protocol::Expose(ChainKeeperRPC::SET_GROUP_NUMBER, controller,
                     &ChainKeeperController::SetGroupNumber);
    Protocol::Expose(ChainKeeperRPC::GROUP_NUMBER, controller,
                     &ChainKeeperController::group_number);
    Protocol::Expose(ChainKeeperRPC::COUNT_OUTGOING_CONNECTIONS, controller,
                     &ChainKeeperController::count_outgoing_connections);

    // Web interface
    auto connect_to = [this](fetch::http::ViewParameters const &params,
                             fetch::http::HTTPRequest const &req) {
      this->ConnectTo(params["ip"], uint16_t(params["port"].AsInt()));
      return fetch::http::HTTPResponse("{\"status\":\"ok\"}");
    };

    HTTPModule::Get(
        "/group-connect-to/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/"
        "(port=\\d+)",
        connect_to);

    auto all_details = [this](fetch::http::ViewParameters const &params,
                              fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;
      response << "{\"outgoing\": [";
      this->with_peers_do(
          [&response](std::vector<client_shared_ptr_type> const &,
                      std::vector<EntryPoint> const &details) {
            bool first = true;
            for (auto &d : details) {
              if (!first) response << ", \n";
              response << "{\n";

              response << "\"group\": " << d.group << ",";
              response << "\"host\": \"" << d.host << "\",";
              response << "\"port\": " << d.port << ",";
              response << "\"http_port\": " << d.http_port << ",";
              response << "\"configuration\": " << d.configuration;

              response << "}";
              first = false;
            }

          });

      response << "],";
      response << "\"transactions\": [";
      this->with_transactions_do(
          [&response](std::vector<transaction_type> const &alltxs) {
            bool first = true;
            std::size_t i = 0;

            for (auto const &t : alltxs) {
              auto sum = t.summary();

              if (!first) response << ", \n";
              response << "{\n";

              bool bfi = true;
              response << "\"groups\": [";
              for (auto &g : sum.groups) {
                if (!bfi) response << ", ";
                response << g;
                bfi = false;
              }
              response << "],";
              response << "\"transaction_number\": " << i << ",";
              response << "\"transaction_hash\": \""
                       << byte_array::ToBase64(sum.transaction_hash) << "\"";
              response << "}";
              first = false;
              ++i;
            }
          });

      response << "]";

      response << "}";
      std::cout << response.str() << std::endl;

      return fetch::http::HTTPResponse(response.str());
    };
    HTTPModule::Get("/all-details", all_details);

    auto list_outgoing = [this](fetch::http::ViewParameters const &params,
                                fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;
      response << "{\"outgoing\": [";
      this->with_peers_do(
          [&response](std::vector<client_shared_ptr_type> const &,
                      std::vector<EntryPoint> const &details) {
            bool first = true;
            for (auto &d : details) {
              if (!first) response << ", \n";
              response << "{\n";

              response << "\"group\": " << d.group << ",";
              response << "\"host\": \"" << d.host << "\",";
              response << "\"port\": " << d.port << ",";
              response << "\"http_port\": " << d.http_port << ",";
              response << "\"configuration\": " << d.configuration;

              response << "}";
              first = false;
            }

          });

      response << "]}";

      std::cout << response.str() << std::endl;

      return fetch::http::HTTPResponse(response.str());
    };
    HTTPModule::Get("/list/outgoing", list_outgoing);

    auto list_transactions = [this](fetch::http::ViewParameters const &params,
                                    fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;
      response << "{\"transactions\": [";
      this->with_transactions_do(
          [&response](std::vector<transaction_type> const &alltxs) {
            bool first = true;
            std::size_t i = 0;

            for (auto const &t : alltxs) {
              auto sum = t.summary();

              if (!first) response << ", \n";
              response << "{\n";

              bool bfi = true;
              response << "\"groups\": [";
              for (auto &g : sum.groups) {
                if (!bfi) response << ", ";
                response << g;
                bfi = false;
              }
              response << "],";
              response << "\"transaction_number\": " << i << ",";
              response << "\"transaction_hash\": \""
                       << byte_array::ToBase64(sum.transaction_hash) << "\"";
              response << "}";
              first = false;
              ++i;
            }
          });

      response << "]}";

      std::cout << response.str() << std::endl;

      return fetch::http::HTTPResponse(response.str());
    };
    HTTPModule::Get("/list/transactions", list_transactions);

    auto submit_transaction = [this, network_manager](
        fetch::http::ViewParameters const &params,
        fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      network_manager->Post([this, req]() {
        json::JSONDocument doc = req.JSON();

        typedef fetch::chain::Transaction transaction_type;
        transaction_type tx;
        auto res = doc["resources"];

        for (std::size_t i = 0; i < res.size(); ++i) {
          auto s = res[i].as_byte_array();
          byte_array::ByteArray group =
              byte_array::FromHex(s.SubArray(2, s.size() - 2));

          tx.PushGroup(group);
        }
        tx.set_arguments(req.body());
        this->PushTransaction(tx);
      });

      return fetch::http::HTTPResponse("{\"status\": \"ok\"}");
    };

    HTTPModule::Post("/group/submit-transaction", submit_transaction);
  }

  uint64_t Ping() {
    LOG_STACK_TRACE_POINT;

    fetch::logger.Debug("Responding to Ping request");
    return 1337;
  }
};
}
}

#endif
