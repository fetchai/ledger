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

#include <iostream>
#include <fstream>

//extern bool initialized = false;

namespace fetch
{
namespace network_benchmark
{

using transaction = chain::Transaction;

fetch::random::LaggedFibonacciGenerator<> lfg;

template< typename T>
void MakeString(T &str, std::size_t N = 256) {
  byte_array::ByteArray entry;
  entry.Resize(N);

  for(std::size_t j=0; j < N; ++j) {
    entry[j] = uint8_t( lfg()  >> 19 );
  }

  str = entry;
}


class Node
{
public:

  //typedef transaction chain::Transaction;

  Node(network::ThreadManager *tm) :
    nodeDirectory_{tm}
  {}

  Node(Node &rhs)            = delete;
  Node(Node &&rhs)           = delete;
  Node operator=(Node& rhs)  = delete;
  Node operator=(Node&& rhs) = delete;

  ~Node()
  {
    if(thread_.joinable())
    {
      thread_.join();
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

  void setTransactionsPerCall(int tpc)
  {
    std::stringstream stream;
    stream << "setting transactions per call to: " << tpc << std::endl;
    std::cerr << stream.str();

    transactionsPerCall_ = tpc;
    fetch::logger.Info("Building...");
    precreateTrans();
    fetch::logger.Info("set transactions per call to ", tpc);
  }

  void Reset()
  {

    {
      std::stringstream stream;
      stream << "stopping..." << std::endl;
      std::cerr << stream.str();

      receivedCount_ = 0;
    }

    std::lock_guard<std::mutex> mlock(mutex_);
    sendingTransactions_ = false;
    packetFilter_.reset();
    transactionList_.reset();

    std::stringstream stream;
    stream << "stopped..." << std::endl;
    std::cerr << stream.str();
  }

  void Start()
  {
    initialized = true;

    std::stringstream stream;
    stream << "starting..." << std::endl;
    std::cerr << stream.str();

    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    std::lock_guard<std::mutex> mlock(mutex_);

    sendingTransactions_ = false;

    if(thread_.joinable())
    {
      thread_.join();
    }

    sendingTransactions_ = true;

    if(isPull_)
    {
      thread_   = std::thread([this]() { pullTransactions();});
    }
    else
    {
      thread_   = std::thread([this]() { sendTransactions();});
    }
  }

  void Stop()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    initialized = false;

    mutex_.lock();
    sendingTransactions_ = false;
    mutex_.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::stringstream stream;
    stream << "Stopping, we sent/pulled: " << transacCount_ << std::endl;
    stream << "We recorded: " << receivedCount_ << std::endl;
    stream << "We logged: " << transactionList_.size() << std::endl;
    std::cerr << stream.str();

    fetch::logger.PrintTimings(50, std::cerr);

    if(thread_.joinable())
    {
      thread_.join();
    }
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
    exit(1);
    //transactionList_.Add(trans);
  }

  std::vector<transaction> const &ProvideTransactions()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    if(premadeTransIndex_ == 0)
    {
      fetch::logger.Error("Ran out of transactions to provide");
      //exit(1);
    }

    std::size_t index = premadeTransIndex_--;

    const std::vector<transaction> &tran = premadeTrans_[index];

    transactionList_.Add(const_cast<std::vector<transaction> *>(&tran));

    fetch::logger.Info("sending trans of size: ", tran.size());

    return tran;
  }

  void ping() { std::cout << "pinged" << std::endl;}

  static transaction NextTransaction()
  {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());

    transaction trans;

    trans.PushGroup( group_type( dis(gen) ) );
    trans.PushGroup( group_type( dis(gen) ) );
    trans.PushGroup( group_type( dis(gen) ) );
    trans.PushGroup( group_type( dis(gen) ) );
    trans.PushGroup( group_type( dis(gen) ) );

    byte_array::ByteArray sig1, sig2, contract_name, arg1;
    MakeString(sig1);
    MakeString(sig2);
    MakeString(contract_name);
    MakeString(arg1, 4 * 256);

    trans.PushSignature(sig1);
    trans.PushSignature(sig2);
    trans.set_contract_name(contract_name);
    trans.set_arguments(arg1);
    trans.UpdateDigest();

    return trans;
  }

private:
  bool                                               isPull_ = true;
  NodeDirectory                                      nodeDirectory_;   // Manage connections to other nodes
  PacketFilter<byte_array::BasicByteArray, 1000>     packetFilter_;    // Filter 'already seen' packets
  TransactionList<transaction, 500000>  transactionList_; // List of all transactions, sent and received
  fetch::mutex::Mutex                                mutex_;

  // Transmitting thread
  std::thread                                        thread_;
  uint32_t                                           threadSleepTimeUs_   = 1000;
  uint32_t                                           transactionsPerCall_ = 1000;
  bool                                               sendingTransactions_ = false;
  std::atomic<std::size_t>                           premadeTransIndex_{0};
  std::size_t                                        transacCount_        = 0;
  std::atomic<std::size_t>                           receivedCount_{0};
  std::vector<std::vector<transaction>>              premadeTrans_;


  void precreateTrans()
  {
    std::lock_guard<std::mutex> mlock(mutex_);
    std::size_t numberToSend      = transactionsPerCall_;
    std::size_t transInVector     = 0;
    std::size_t vectorInPremade   = 0;
    std::size_t index             = 0;

    // Keep everything as small as possible at cost of slow setup time
    premadeTrans_.clear();

    while (transInVector < 500000)
    {
      if(!(index < premadeTrans_.size()))
      {
        premadeTrans_.resize(index+1);
      }

      std::vector<transaction> &transBlock = premadeTrans_[index++];
      transBlock.resize(numberToSend);

      for (std::size_t i = 0; i < numberToSend; ++i)
      {
        auto trans = NextTransaction();
        transBlock[i] = trans;
      }

      transInVector += numberToSend;
      vectorInPremade++;
    }

    premadeTrans_.resize(vectorInPremade);
    premadeTransIndex_ = premadeTrans_.size()-1;
  }

  std::vector<transaction> getTrans(std::size_t numberToSend)
  {
    std::vector<transaction> allTrans;

    bool make_transaction = false;

    if(!make_transaction && premadeTrans_.size() > 0)
    {
      allTrans = premadeTrans_[premadeTrans_.size()-1];
      premadeTrans_.pop_back();
    }
    else
    {
      fetch::logger.Info(std::cerr, "Ran out of transactions");

      for (std::size_t i = 0; i < numberToSend; ++i)
      {
        auto trans = NextTransaction();
        allTrans.push_back(trans);
      }
    }

    return allTrans;
  }

  void sendTransactions()
  {
    std::chrono::microseconds sleepTime(threadSleepTimeUs_);
    transacCount_ = 0;
    std::size_t numberToSend = transactionsPerCall_;

    while(sendingTransactions_)
    {
      LOG_STACK_TRACE_POINT;

      const std::vector<transaction> &allTrans = getTrans(numberToSend);

      transacCount_ += numberToSend;
      if(transacCount_ % 1000 == 0) {std::cerr << ".";}

      //transactionList_.Add(allTrans);
      nodeDirectory_.BroadcastTransactions(allTrans);

      if(threadSleepTimeUs_ > 0)
      {
        std::this_thread::sleep_for(sleepTime);
      }
    }

    precreateTrans();
  }

  void pullTransactions()
  {
    std::chrono::microseconds sleepTime(threadSleepTimeUs_);
    transacCount_ = 0;
    std::size_t numberToSend = transactionsPerCall_;

    while(sendingTransactions_)
    {
      LOG_STACK_TRACE_POINT;

      std::vector<std::vector<transaction>> allTrans = nodeDirectory_.RequestTransactions();

      if (!sendingTransactions_)
      {
        break;
      }

      for(auto &i : allTrans)
      {
        transacCount_ += i.size();
      }

      for(auto &i : allTrans)
      {
        fetch::logger.Info("received trans of size: ", i.size());
        transactionList_.Add(std::move(i));
      }

      if(threadSleepTimeUs_ > 0)
      {
        std::this_thread::sleep_for(sleepTime);
      }
    }

    precreateTrans();
  }

};

}
}

#endif
