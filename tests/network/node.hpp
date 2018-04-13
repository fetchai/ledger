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

  // HTTP calls
  void addEndpoint(const Endpoint &endpoint)
  {
    nodeDirectory_.addEndpoint(endpoint);
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
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    //fetch::log::Context log_context(__FUNCTION_NAME__, __FILE__, __LINE__, this); 

    std::unique_lock<std::mutex> mlock(mutex_);

    keepCount = 0;

    // Set up the transaction generator
    transactionGenerator_.eventPeriodUs(rate_);

    if(setup_ == false)
    {
      setup_ = true;
      transactionGenerator_.event([this]()
      {
        LOG_STACK_TRACE_POINT_WITH_INSTANCE;
        auto trans = nextTransaction();

        auto &hash = trans.summary().transaction_hash;

        if(true || packetFilter_.Add(hash))
        {
          if(keepCount++ % 1000 == 0) {std::cerr << ".";}

          transactionList_.Add(trans);
          nodeDirectory_.BroadcastTransaction(std::move(trans));
        }
        else
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
          std::cout << "Trans blocked" << std::endl;
        }

      });
    }
    transactionGenerator_.start();
  }

  void Stop()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    std::cerr << "Stopping, we sent: " << keepCount << std::endl;
    std::cerr << "We recorded: " << transactionList_.size() << std::endl;
    transactionGenerator_.stop();
    fetch::logger.PrintTimings();
  }

  std::set<chain::Transaction> GetTransactions()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    return transactionList_.GetTransactions();
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    return transactionList_.TransactionsHash();
  }

  // RPC calls
  void ReceiveTransaction(chain::Transaction trans)
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    transactionList_.Add(std::move(trans));
  }

  void ping() { std::cout << "pinged" << std::endl;}

private:
  int                                            seed_;
  NodeDirectory                                  nodeDirectory_;                 // Manage connections to other nodes
  PacketFilter<byte_array::BasicByteArray, 1000> packetFilter_;                  // Filter 'already seen' packets
  EventGenerator                                 transactionGenerator_;          // Generate transactions at a certain rate
  TransactionList<chain::Transaction, 500000>    transactionList_;               // List of all transactions, sent and received
  int                                            rate_ = 100;
  int                                            keepCount = 0;
  std::mutex                                     mutex_;
  bool                                           setup_ = false;

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

};

}
}

#endif
