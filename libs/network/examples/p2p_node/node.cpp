#include "core/byte_array/consumers.hpp"
#include "core/commandline/cli_header.hpp"
#include "core/commandline/parameter_parser.hpp"
#include "network/p2pservice/p2p_service.hpp"
#include "network/service/server.hpp"
#include <iostream>

#include <sstream>
using namespace fetch;
using namespace fetch::p2p;
using namespace fetch::byte_array;
enum
{
  TOKEN_NAME      = 1,
  TOKEN_STRING    = 2,
  TOKEN_NUMBER    = 3,
  TOKEN_CATCH_ALL = 12
};

int main(int argc, char **argv)
{
  // Reading config
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  uint16_t port = params.GetParam<uint16_t>("port", 8080);

  int log = params.GetParam<int>("showlog", 0);
  if (log == 0)
  {
    fetch::logger.DisableLogger();
  }

  fetch::commandline::DisplayCLIHeader("P2P Service");

  // Setting up
  fetch::network::NetworkManager tm(8);
  P2PService                     service(port, tm);

  tm.Start();

  // Taking commands
  std::string            line = "";
  Tokenizer              tokenizer;
  std::vector<ByteArray> command;

  tokenizer.AddConsumer(consumers::StringConsumer<TOKEN_STRING>);
  tokenizer.AddConsumer(consumers::NumberConsumer<TOKEN_NUMBER>);
  tokenizer.AddConsumer(consumers::Token<TOKEN_NAME>);
  tokenizer.AddConsumer(consumers::AnyChar<TOKEN_CATCH_ALL>);

  while ((std::cin) && (line != "quit"))
  {
    std::cout << ">> ";

    // Getting command
    std::getline(std::cin, line);
    try
    {

      command.clear();
      tokenizer.clear();
      tokenizer.Parse(line);

      for (auto &t : tokenizer)
      {
        if (t.type() != TOKEN_CATCH_ALL) command.push_back(t);
      }

      if (command.size() > 0)
      {
        if (command[0] == "connect")
        {
          if (command.size() != 3)
          {
            std::cout << "usage: connect [host] [port]" << std::endl;
            continue;
          }
          service.Connect(command[1], uint16_t(command[2].AsInt()));
        }

        if (command[0] == "publish_profile")
        {
          if (command.size() != 1)
          {
            std::cout << "usage: publish_profile" << std::endl;
            continue;
          }
          service.PublishProfile();
        }

        if (command[0] == "test")
        {
          if (command.size() != 1)
          {
            std::cout << "usage: test" << std::endl;
            continue;
          }
          std::cout << "addmc mainchain " << port + 1 << std::endl;
          std::cout << "addl 0 lane0 " << port + 2 << std::endl;
          std::cout << "addl 1 lane0 " << port + 3 << std::endl;
          std::cout << "publish_profile " << std::endl;
          service.AddMainChain("mainchain", port + 1);
          service.AddLane(0, "lane0", port + 1);
          service.AddLane(1, "lane1", port + 2);
          service.PublishProfile();

          //          service.Connect(command[1], uint16_t(command[2].AsInt()));
        }

        if (command[0] == "addl")
        {
          if (command.size() != 4)
          {
            std::cout << "usage: addl [lane] [host] [port]" << std::endl;
            continue;
          }
          service.AddLane(uint32_t(command[1].AsInt()), command[2], uint16_t(command[3].AsInt()));
        }

        if (command[0] == "addmc")
        {
          if (command.size() != 3)
          {
            std::cout << "usage: addmc [host] [port]" << std::endl;
            continue;
          }
          service.AddMainChain(command[1], uint16_t(command[2].AsInt()));
        }

        if (command[0] == "needpeers")
        {
          if (command.size() != 1)
          {
            std::cout << "usage: needpeers" << std::endl;
            continue;
          }
          service.RequestPeers();
        }

        if (command[0] == "enoughpeers")
        {
          if (command.size() != 1)
          {
            std::cout << "usage: enoughpeers" << std::endl;
            continue;
          }
          service.EnoughPeers();
        }

        if (command[0] == "suggest")
        {
          if (command.size() != 1)
          {
            std::cout << "usage: suggest" << std::endl;
            continue;
          }
          auto map = service.SuggestPeersToConnectTo();
          std::cout << "Suggestions:" << std::endl;

          for (auto &me : map)
          {
            auto pd = me.second;

            std::cout << "Peer: " << byte_array::ToBase64(pd.identity.identifier()) << std::endl;
            for (auto &e : pd.entry_points)
            {
              std::cout << "  - ";
              for (auto &h : e.host)
              {
                std::cout << h << " ";
              }
              std::cout << ":" << e.port << " > ";
              if (e.is_discovery)
              {
                std::cout << "DISCOVERY ";
              }
              if (e.is_mainchain)
              {
                std::cout << "MAIN CHAIN ";
              }
              if (e.is_lane)
              {
                std::cout << "LANE " << e.lane_id << " ";
              }
              std::cout << std::endl;
            }
          }
        }

        if (command[0] == "list")
        {
          if (command.size() != 1)
          {
            std::cout << "usage: list" << std::endl;
            continue;
          }
          auto reg = service.connection_register();
          using details_map_type =
              typename fetch::network::ConnectionRegister<PeerDetails>::details_map_type;

          reg.WithClientDetails([](details_map_type map) {
            std::cout << "Lising peers" << std::endl;
            for (auto &me : map)
            {
              auto pd = me.second;

              std::lock_guard<mutex::Mutex> lock(*pd);
              std::cout << "Peer: " << byte_array::ToBase64(pd->identity.identifier()) << std::endl;
              for (auto &e : pd->entry_points)
              {
                std::cout << "  - ";
                for (auto &h : e.host)
                {
                  std::cout << h << " ";
                }
                std::cout << ":" << e.port << " > ";
                if (e.is_discovery)
                {
                  std::cout << "DISCOVERY ";
                }
                if (e.is_mainchain)
                {
                  std::cout << "MAIN CHAIN ";
                }
                if (e.is_lane)
                {
                  std::cout << "LANE " << e.lane_id << " ";
                }
                std::cout << byte_array::ToBase64(e.identity.identifier());

                std::cout << std::endl;
              }
            }
          });
        }
      }
    }
    catch (serializers::SerializableException &e)
    {
      std::cerr << "error: " << e.what() << std::endl;
    }
  }

  tm.Stop();

  return 0;
}
