#ifndef NETWORK_TEST_NODE_HPP
#define NETWORK_TEST_NODE_HPP

// This represents the API to the network test
#include"chain/transaction.hpp"
#include"byte_array/basic_byte_array.hpp"
#include"random/lfg.hpp"
#include"./network_classes.hpp"
#include"./node_directory.hpp"
#include"./packet_filter.hpp"
#include"./event_generator.hpp"
#include"./transaction_list.hpp"

namespace fetch
{
namespace network_test
{

class Node
{
public:
  Node(network::ThreadManager *tm, int seed) :
    seed_{seed},
    nodeDirectory_{tm}
  {}

  Node(Node &rhs)            = delete;
  Node(Node &&rhs)           = delete;
  Node operator=(Node& rhs)  = delete;
  Node operator=(Node&& rhs) = delete;

  void ping() { std::cout << "pinged" << std::endl;}

  template <typename T>
  std::string toHex(T&& t)
  {
    std::string temp = t;
    for(auto &i : temp)
    {
      if(i&0xFF <= 9)
      {
        i = (i&0xFF) + 48;
      }
    }

    return temp;
  }

  chain::Transaction nextTransaction()
  {
    static fetch::random::LaggedFibonacciGenerator<> lfg(seed_);
    chain::Transaction trans;

    // Push on two 32-bit numbers so as to avoid multiple nodes creating duplicates
    trans.PushGroup(lfg());
    trans.PushGroup(lfg());
    trans.UpdateDigest();

    return trans;
  }

  void setRate(int rate)
  {
    std::cerr << "Setting rate to: " << rate << std::endl;
    rate_ = rate;
  }

  void Reset()
  {
    std::cerr << "stopping..." << std::endl;

    transactionGenerator_.stop();
    packetFilter_.reset();
    transactionList_.reset();

    std::cerr << "stopped..." << std::endl;
  }

  void Start()
  {

    std::cerr << "starting..." << std::endl;

    // Set up the transaction generator
    transactionGenerator_.eventPeriodUs(rate_);
    transactionGenerator_.event([this]()
    {
      auto trans = nextTransaction();

      auto &hash = trans.summary().transaction_hash;

      if(packetFilter_.Add(trans.summary().transaction_hash))
      {
        static int sent = 0;
        if(sent++ % 1000) {std::cerr << ".";}

        transactionList_.Add(trans);
        nodeDirectory_.BroadcastTransaction(std::move(trans));
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "Trans blocked" << std::endl;
      }

    });
    transactionGenerator_.start();
  }

  void Stop()
  {
    transactionGenerator_.stop();
  }

  // HTTP calls
  void addEndpoint(const Endpoint &endpoint)
  {
    nodeDirectory_.addEndpoint(endpoint);
  }

  void ReceiveTransaction(chain::Transaction trans)
  {
    transactionList_.Add(std::move(trans));
  }

  std::set<chain::Transaction> GetTransactions()
  {
    return transactionList_.GetTransactions();
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    return transactionList_.TransactionsHash();
  }

private:
  NodeDirectory                                  nodeDirectory_;                 // Manage connections to other nodes
  PacketFilter<byte_array::BasicByteArray, 1000> packetFilter_;                  // Filter 'already seen' packets
  EventGenerator                                 transactionGenerator_;          // Generate transactions at a certain rate
  TransactionList<chain::Transaction, 5000000>   transactionList_;               // List of all transactions, sent and received
  int                                            seed_;
  int                                            rate_ = 100;
};

}
}

#endif
