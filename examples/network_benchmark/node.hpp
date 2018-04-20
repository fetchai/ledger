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

#include<random>
#include<memory>
#include<utility>
#include<limits>
#include<set>

namespace fetch
{
namespace network_benchmark
{

class Node
{
public:

  using transaction = chain::Transaction;

  Node(network::ThreadManager *tm, int seed) :
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

  void setRate(uint32_t rate)
  {
    std::stringstream stream;
    stream << "Setting rate to: " << rate << std::endl;
    std::cerr << stream.str();

    threadSleepTimeUs_ = rate;
  }

  void setTransactionsPerCall(uint32_t tpc)
  {
    std::stringstream stream;
    stream << "Setting transactions per call to: " << tpc << std::endl;
    std::cerr << stream.str();

    transactionsPerCall_ = tpc;
  }

  void Reset()
  {
    {
      std::stringstream stream;
      stream << "stopping..." << std::endl;
      std::cerr << stream.str();

      receivedCount_ = 0;
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
    stream << "Stopping, we sent: " << sentCount_ << std::endl;
    stream << "We recorded: " << receivedCount_ << std::endl;
    stream << "We logged: " << transactionList_.size() << std::endl;
    std::cerr << stream.str();

    fetch::logger.PrintTimings();
  }

  std::set<transaction> GetTransactions()
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
  void ReceiveTransactions(std::vector<transaction> trans)
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    receivedCount_ += trans.size();
    transactionList_.Add(trans);
  }

  void ping() { std::cout << "pinged" << std::endl;}

private:
  NodeDirectory                                      nodeDirectory_;   // Manage connections to other nodes
  PacketFilter<byte_array::BasicByteArray, 1000>     packetFilter_;    // Filter 'already seen' packets
  TransactionList<transaction, 500000>               transactionList_; // List of all transactions, sent and received
  std::mutex                                         mutex_;

  // Transmitting thread
  std::unique_ptr<std::thread>                       thread_;
  uint32_t                                           threadSleepTimeUs_   = 1000;
  uint32_t                                           transactionsPerCall_ = 1000;
  bool                                               sendingTransactions_ = false;
  std::size_t                                        sentCount_           = 0;
  std::atomic<std::size_t>                           receivedCount_{0};

  transaction nextTransaction()
  {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());

    transaction trans;

    // Push on two 32-bit numbers so as to avoid multiple nodes creating duplicates
    trans.PushGroup(group_type( dis(gen) ));
    trans.PushGroup(group_type( dis(gen) ));
    trans.UpdateDigest();

    return trans;
  }

  std::vector<transaction> getTrans(std::size_t numberToSend)
  {
    std::vector<transaction> allTrans;

    for (std::size_t i = 0; i < numberToSend; ++i)
    {
      auto trans = nextTransaction();
      allTrans.push_back(trans);
    }

    return allTrans;
  }

  void sendTransactions()
  {
    std::chrono::microseconds sleepTime(threadSleepTimeUs_);
    sentCount_ = 0;

    while(sendingTransactions_)
    {
      LOG_STACK_TRACE_POINT;
      std::size_t numberToSend = transactionsPerCall_;

      const std::vector<transaction> &allTrans = getTrans(numberToSend);

      sentCount_ += numberToSend;
      if(sentCount_ % 1000 == 0) {std::cerr << ".";}

      transactionList_.Add(allTrans);
      nodeDirectory_.BroadcastTransactions(allTrans);

      if(threadSleepTimeUs_ > 0)
      {
        std::this_thread::sleep_for(sleepTime);
      }
    }
  }

};

}
}

#endif
