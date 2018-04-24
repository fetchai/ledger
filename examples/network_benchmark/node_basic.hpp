#ifndef NETWORK_BENCHMARK_NODE_BASIC_HPP
#define NETWORK_BENCHMARK_NODE_BASIC_HPP

// This represents the API to the network test
#include"chain/transaction.hpp"
#include"byte_array/basic_byte_array.hpp"
#include"random/lfg.hpp"
#include"./network_classes.hpp"
#include"./network_functions.hpp"
#include"./node_directory.hpp"
#include"./transaction_list.hpp"
#include"logger.hpp"

#include<random>
#include<memory>
#include<utility>
#include<limits>
#include<set>
#include<chrono>
#include<ctime>
#include<stdlib.h>

#include <iostream>
#include <fstream>

namespace fetch
{
namespace network_benchmark
{

class NodeBasic
{
public:

  NodeBasic(network::ThreadManager *tm) :
    nodeDirectory_{tm}
  {}

  NodeBasic(NodeBasic &rhs)            = delete;
  NodeBasic(NodeBasic &&rhs)           = delete;
  NodeBasic operator=(NodeBasic& rhs)  = delete;
  NodeBasic operator=(NodeBasic&& rhs) = delete;

  ~NodeBasic()
  {
    if(thread_.joinable())
    {
      thread_.join();
    }
  }

  ///////////////////////////////////////////////////////////
  // HTTP calls for setup
  void AddEndpoint(const Endpoint &endpoint)
  {
    LOG_STACK_TRACE_POINT ;
    fetch::logger.Info(std::cerr, "Adding endpoint");
    nodeDirectory_.AddEndpoint(endpoint);
  }

  void setRate(int rate)
  {
    LOG_STACK_TRACE_POINT ;
    fetch::logger.Info(std::cerr, "Setting rate to: ", rate);
    threadSleepTimeUs_ = rate;
  }

  void SetTransactionsPerCall(int tpc)
  {
    LOG_STACK_TRACE_POINT ;
    transactionsPerCall_ = tpc;
    fetch::logger.Info(std::cerr, "set transactions per call to ", tpc);
  }

  void SetTransactionsToSync(uint32_t transactionsToSync)
  {
    LOG_STACK_TRACE_POINT ;
    fetch::logger.Info(std::cerr, "set transactions per call to ", transactionsToSync);
    fetch::logger.Info(std::cerr, "Building...");
    PrecreateTrans(transactionsToSync);
    AddTransToList();
  }

  void setStopCondition(uint32_t stopCondition)
  {
    LOG_STACK_TRACE_POINT ;
    stopCondition_ = stopCondition;
  }

  void setStartTime(uint64_t startTime)
  {
    LOG_STACK_TRACE_POINT ;
    fetch::logger.Info(std::cerr, "setting start time to ", startTime);
    startTime_ = startTime;

    if(thread_.joinable())
    {
      thread_.join();
    }

    thread_   = std::thread([this]() { pullTransactions();});
  }

  double timeToComplete()
  {
    LOG_STACK_TRACE_POINT ;
    return std::chrono::duration_cast<std::chrono::duration<double>>
      (finishTimePoint_ - startTimePoint_).count();
  }

  void Reset()
  {
    LOG_STACK_TRACE_POINT ;
    transactionList_.reset();
    provideIndex = 0;
  }

  void Start()
  {
  }

  void Stop()
  {
  }

  bool finished() const
  {    
    return finished_;
  }

  uint32_t TransactionSize() const
  {
    LOG_STACK_TRACE_POINT ;
    auto trans = NextTransaction();
    serializers::TypedByte_ArrayBuffer serializer;
    serializer << trans;
    return serializer.size();
  }

  ///////////////////////////////////////////////////////////
  // Thread attempting syncronisation
  void pullTransactions()
  {
    LOG_STACK_TRACE_POINT ;
    // get time as epoch, wait until that time to start
    std::time_t t = startTime_;
    std::tm* timeout_tm = std::localtime(&t);

    time_t timeout_time_t=mktime(timeout_tm);
    std::chrono::system_clock::time_point timeout_tp = std::chrono::system_clock::from_time_t(timeout_time_t);
    std::this_thread::sleep_until(timeout_tp);

    startTimePoint_ = std::chrono::high_resolution_clock::now();
    fetch::logger.Info("Starting sync.");
    finished_ = false;

    while(1)
    {
      std::vector<std::vector<transaction>> allTrans = nodeDirectory_.RequestNextBlock(threadSleepTimeUs_);

      finishTimePoint_ = std::chrono::high_resolution_clock::now();

      if(allTrans.size() == 0)
      {
        break;
      }

      for(auto &i : allTrans)
      {
        fetch::logger.Info("received trans of size: ", i.size());
        transactionList_.Add(std::move(i));
      }
    }

    //finishTimePoint_ = std::chrono::high_resolution_clock::now();
    fetch::logger.Info("finished.");
    finished_ = true;

    std::cerr << std::chrono::duration_cast<std::chrono::duration<double>> (finishTimePoint_ - startTimePoint_).count() << std::endl;
  }


  ///////////////////////////////////////////////////////////
  // RPC calls
  void ReceiveTransactions(std::vector<transaction> trans) {} // TODO: (`HUT`) : remove

  std::atomic<std::size_t> provideIndex{0};

  std::vector<transaction> ProvideTransactions(uint32_t thing)
  {
    LOG_STACK_TRACE_POINT ;
    std::size_t thisIndex = provideIndex++;

    fetch::logger.Info("Called with thisIndex: ", thisIndex);
    if(!(thisIndex < premadeTransSize_))
    {
      fetch::logger.Info("thisIndex too large!");
      return emptyVector_;
    }

    const std::vector<transaction> &tran = premadeTrans_[thisIndex];
    return tran;
  }

  ///////////////////////////////////////////////////////////
  // Functions to check that syncronisation was successful
  std::set<transaction> GetTransactions()
  {
    LOG_STACK_TRACE_POINT ;
    return transactionList_.GetTransactions();
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    LOG_STACK_TRACE_POINT ;
    return transactionList_.TransactionsHash();
  }

private:
  NodeDirectory                         nodeDirectory_;   // Manage connections to other nodes
  TransactionList<transaction, 500000>  transactionList_; // List of all transactions, sent and received
  fetch::mutex::Mutex                   mutex_;

  // Transmitting thread
  std::thread                                        thread_;
  uint32_t                                           threadSleepTimeUs_   = 1000;
  uint32_t                                           transactionsPerCall_ = 1000;
  std::vector<std::vector<transaction>>              premadeTrans_;
  std::vector<transaction>                           emptyVector_;
  std::size_t                                        premadeTransSize_;
  uint32_t                                           stopCondition_{0};
  long int                                           startTime_{0};
  float                                              timeToComplete_{0.0};
  std::chrono::high_resolution_clock::time_point     startTimePoint_  = std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point     finishTimePoint_ = std::chrono::high_resolution_clock::now();
  bool                                               finished_ = false;

  void PrecreateTrans(uint32_t total)
  {
    std::lock_guard<std::mutex> mlock(mutex_);
    std::size_t numberToSend      = transactionsPerCall_;
    std::size_t transInVector     = 0;
    std::size_t index             = 0;

    if(total % numberToSend != 0)
    {
      fetch::logger.Info(std::cerr, "Transactions per call not a multiple of total!");
      exit(1);
    }

    premadeTrans_.clear();

    while (transInVector < total)
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
    }

    premadeTransSize_ = premadeTrans_.size();
  }

  void AddTransToList()
  {
    for(auto &i : premadeTrans_)
    {
      transactionList_.Add(&i);
    }
  }

};

}
}

#endif
