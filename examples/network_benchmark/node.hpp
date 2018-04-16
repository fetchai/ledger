#ifndef NETWORK_BENCHMARK_NODE_HPP
#define NETWORK_BENCHMARK_NODE_HPP

// This represents the API to the network test
#include"chain/transaction.hpp"
#include"byte_array/basic_byte_array.hpp"
#include"random/lfg.hpp"
#include"./network_classes.hpp"
#include"./node_directory.hpp"
#include"./packet_filter.hpp"
#include"./transaction_list.hpp"

namespace fetch
{
namespace network_benchmark
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

  ~Node()
  {
    if(thread_)
    {
      thread_->join();
    }
  }

  // HTTP calls
  void addEndpoint(const Endpoint &endpoint)
  {
    nodeDirectory_.addEndpoint(endpoint);
  }

  void setRate(int rate)
  {
    std::stringstream stream;
    stream << "Setting rate to: " << rate << std::endl;
    std::cerr << stream.str();

    threadSleepTimeUs_ = rate;
  }

  void Reset()
  {
    {
      std::stringstream stream;
      stream << "stopping..." << std::endl;
      std::cerr << stream.str();
    }


    std::unique_lock<std::mutex> mlock(mutex_);
    sendingTransactions_ = false;
    packetFilter_.reset();
    transactionList_.reset();

    std::stringstream stream;
    stream << "stopped..." << std::endl;
    std::cerr << stream.str();


  }

  void Start()
  {
    std::stringstream stream;
    stream << "starting..." << std::endl;
    std::cerr << stream.str();

    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    std::unique_lock<std::mutex> mlock(mutex_);

    sendingTransactions_ = false;

    if(thread_)
    {
      thread_->join();
    }

    sendingTransactions_ = true;

    thread_   = std::unique_ptr<std::thread>(new std::thread([this]() { sendTransactions();}));
  }

  void Stop()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    mutex_.lock();
    sendingTransactions_ = false;
    mutex_.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::stringstream stream;
    stream << "Stopping, we sent: " << keepCount << std::endl;
    stream << "We recorded: " << transactionList_.size() << std::endl;
    std::cerr << stream.str();

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
    std::stringstream stream;
    stream << "Received new transaction" << std::endl;
    std::cerr << stream.str();

    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    transactionList_.Add(std::move(trans));
  }

  void ping() { std::cout << "pinged" << std::endl;}

private:
  int                                            seed_;
  NodeDirectory                                  nodeDirectory_;                 // Manage connections to other nodes
  PacketFilter<byte_array::BasicByteArray, 1000> packetFilter_;                  // Filter 'already seen' packets
  TransactionList<chain::Transaction, 500000>    transactionList_;               // List of all transactions, sent and received
  std::mutex                                     mutex_;

  // Transmitting thread
  std::unique_ptr<std::thread>                   thread_;
  uint32_t                                       threadSleepTimeUs_ = 1000;
  bool                                           sendingTransactions_ = false;
  int                                            keepCount = 0;

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

  void sendTransactions()
  {
    //std::chrono::microseconds sleepTime(threadSleepTimeUs_);
    //keepCount = 0;

//    while(sendingTransactions_)
//    {
//      LOG_STACK_TRACE_POINT;
//
//      std::stringstream stream;
//      stream << "sending trans" << std::endl;
//      std::cerr << stream.str();
//
//      auto trans = nextTransaction();
//      auto &hash = trans.summary().transaction_hash;
//
//      if(true || packetFilter_.Add(hash))
//      {
//        if(keepCount++ % 1000 == 0) {std::cerr << ".";}
//
//        //transactionList_.Add(trans);
//        //nodeDirectory_.BroadcastTransaction(std::move(trans));
//        {
//          std::stringstream stream;
//          stream << "push to nodedir" << std::endl;
//          std::cerr << stream.str();
//        }
//
//        nodeDirectory_.BroadcastTransaction(trans);
//
//        {
//          std::stringstream stream;
//          stream << "trans sent" << std::endl;
//          std::cerr << stream.str();
//        }
//
//      }
//      else
//      {
//        std::cout << "Trans blocked" << std::endl;
//      }
//
//
    while(true)
    {
      std::chrono::microseconds sleepTime(threadSleepTimeUs_);
      keepCount = 0;
      std::this_thread::sleep_for(sleepTime);

      {
        std::stringstream stream;
        stream << "sending a trans" << std::endl;
        std::cerr << stream.str();
      }

      nodeDirectory_.BroadcastTransaction();

      {
        std::stringstream stream;
        stream << "sent a trans" << std::endl;
        std::cerr << stream.str();
      }
    }
  }

};

}
}

#endif
