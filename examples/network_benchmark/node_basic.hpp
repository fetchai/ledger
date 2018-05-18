#ifndef NETWORK_BENCHMARK_NODE_BASIC_HPP
#define NETWORK_BENCHMARK_NODE_BASIC_HPP

// This represents the API to the network test
#include<stdlib.h>
#include<random>
#include<memory>
#include<utility>
#include<limits>
#include<set>
#include<chrono>
#include<ctime>
#include<iostream>
#include<fstream>
#include<vector>

#include"chain/transaction.hpp"
#include"byte_array/basic_byte_array.hpp"
#include"random/lfg.hpp"
#include"./network_classes.hpp"
#include"./node_directory.hpp"
#include"./transaction_list.hpp"
#include"logger.hpp"
#include"../tests/include/helper_functions.hpp"

namespace fetch
{
namespace network_benchmark
{

class NodeBasic
{
public:
  explicit NodeBasic(network::ThreadManager *tm) :
    nodeDirectory_{tm}
  {}

  NodeBasic(NodeBasic &rhs)            = delete;
  NodeBasic(NodeBasic &&rhs)           = delete;
  NodeBasic operator=(NodeBasic& rhs)  = delete;
  NodeBasic operator=(NodeBasic&& rhs) = delete;

  ~NodeBasic()
  {
    destructing_ = true;

    if (thread_.joinable())
    {
      thread_.join();
    }

    if (forwardQueueThread_.joinable())
    {
      forwardQueueThread_.join();
    }
  }

  ///////////////////////////////////////////////////////////
  // HTTP calls for setup
  void AddEndpoint(const Endpoint &endpoint)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<std::mutex> mlock(mutex_);
    fetch::logger.Info("Adding endpoint");
    nodeDirectory_.AddEndpoint(endpoint);
  }

  void setTransactionsPerCall(uint64_t tpc)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<std::mutex> mlock(mutex_);
    transactionsPerCall_ = tpc;
    fetch::logger.Info("set transactions per call to ", tpc);
  }

  void SetTransactionsToSync(uint64_t transactionsToSync)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<std::mutex> mlock(mutex_);
    fetch::logger.Info("set transactions to sync to ", transactionsToSync);
    fetch::logger.Info("Building...");
    PrecreateTrans(transactionsToSync);
    AddTransToList();
  }

  void setStopCondition(uint64_t stopCondition)
  {
    LOG_STACK_TRACE_POINT;
    stopCondition_ = stopCondition;
  }

  void setStartTime(uint64_t startTime)
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Info("setting start time to ", startTime);
    startTime_ = startTime;

    if (thread_.joinable())
    {
      thread_.join();
    }

    thread_   = std::thread([this]() { SendTransactions();});
  }

  double timeToComplete()
  {
    LOG_STACK_TRACE_POINT;
    return std::chrono::duration_cast<std::chrono::duration<double>>
      (finishTimePoint_ - startTimePoint_).count();
  }

  void Reset()
  {
    LOG_STACK_TRACE_POINT;
    transactionList_.reset();
    nodeDirectory_.Reset();
    finished_ = false;
  }

  void Start()
  {
  }

  void Stop()
  {
  }

  bool finished() const
  {
    std::cerr << "Trans list: " << transactionList_.size()
      << " of " << stopCondition_ << std::endl;
    return finished_;
  }

  void setTransactionSize(uint32_t transactionSize)
  {
    std::size_t baseTxSize = common::Size(common::NextTransaction<transaction_type>(0));
    int32_t     pad        = (int32_t(transactionSize) - int32_t(baseTxSize));
    if (pad < 0)
    {
      fetch::logger.Info("Failed to set tx size to: ", transactionSize, ". Less than base size: ",
          baseTxSize);
      exit(1);
    }
    txPad_ = uint32_t(pad);
  }

  ///////////////////////////////////////////////////////////
  // RPC calls

  // Nodes will invite this node to be pushed their transactions
  bool InvitePush(block_hash const &hash)
  {
    fetch::logger.Info("Responding to invite: ", !transactionList_.Contains(hash));
    return !transactionList_.Contains(hash);
  }

  void Push(network_block const &netBlock)
  {
    static std::atomic<int> pushcount{0};
    int pushClocal = pushcount++;
    fetch::logger.Info("Received push ", pushClocal);

    std::unique_lock<std::mutex> lock(forwardQueueMutex_);
    forwardQueue_.push_back(std::move(netBlock));
    //forwardQueue_.push_back(netBlock);
    forwardQueueCond_.notify_one();

    fetch::logger.Info("Processed push ", pushClocal);
  }

  int ping()
  {
    return 4;
  }

  ///////////////////////////////////////////////////////////
  // Functions to check that synchronisation was successful
  std::set<transaction_type> GetTransactions()
  {
    LOG_STACK_TRACE_POINT;
    return transactionList_.GetTransactions();
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Info("Get hash");
    return transactionList_.TransactionsHash();
  }

private:
  NodeDirectory                              nodeDirectory_;   // Manage connections to other nodes
  TransactionList<block_hash, block> transactionList_; // List of all transactions, sent and received
  fetch::mutex::Mutex                        mutex_;

  // Transmitting thread
  std::thread                                    thread_;
  uint64_t                                       transactionsPerCall_ = 1000;
  uint32_t                                       txPad_               = 0;
  std::vector<network_block>                     premadeTrans_;
  std::size_t                                    premadeTransSize_;
  uint64_t                                       stopCondition_{0};
  uint64_t                                       startTime_{0};
  float                                          timeToComplete_{0.0};
  std::chrono::high_resolution_clock::time_point startTimePoint_      = std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point finishTimePoint_     = std::chrono::high_resolution_clock::now();
  bool                                           finished_            = false;
  bool                                           destructing_         = false;

  std::mutex                                 forwardQueueMutex_;
  mutable std::condition_variable            forwardQueueCond_;
  std::thread                                forwardQueueThread_{[this]() { ForwardThread();}};
  std::vector<network_block>                 forwardQueue_;


  void PrecreateTrans(uint64_t total)
  {
    if (total % transactionsPerCall_ != 0)
    {
      std::cerr << "Incorrect setup." << std::endl;
      std::cerr << "Total not integer multiple of tx per call. Exiting." << std::endl;
      exit(1);
    }

    std::size_t blocks = total / transactionsPerCall_;

    premadeTrans_.resize(blocks);
    premadeTransSize_ = blocks; // TODO: (`HUT`) : delete

    for (uint64_t i = 0; i < blocks; ++i)
    {
      network_block &transBlock = premadeTrans_[i];
      transBlock.second.resize(transactionsPerCall_);

      fetch::logger.Info("Building block ", i);
      for (std::size_t j = 0; j < transactionsPerCall_; j++)
      {
        transBlock.second[j] = common::NextTransaction<transaction_type>(txPad_);
      }

      // Use the first Tx for the block hash, should be adequate to avoid collisions (sha256)
      transBlock.second[0].UpdateDigest();
      transBlock.first = common::Hash(transBlock.second[0].summary().transaction_hash);
    }
  }

  void AddTransToList()
  {
    for (auto &netBlock : premadeTrans_)
    {
      transactionList_.Add(netBlock);
    }
  }

  ///////////////////////////////////////////////////////////
  // Threads
  void SendTransactions()
  {
    LOG_STACK_TRACE_POINT;
    // get time as epoch, wait until that time to start
    std::time_t t = startTime_;
    std::tm* timeout_tm = std::localtime(&t);

    time_t timeout_time_t = mktime(timeout_tm);
    std::chrono::system_clock::time_point timeout_tp =
      std::chrono::system_clock::from_time_t(timeout_time_t);

    // Start all nodes simultaneously
    std::this_thread::sleep_until(timeout_tp);
    startTimePoint_ = std::chrono::high_resolution_clock::now();

    fetch::logger.Info("Starting sync.");
    finished_ = false;

    for (auto &i : premadeTrans_)
    {
      //transactionList_.Add(i);
      fetch::logger.Info("Inviting... ");
      nodeDirectory_.InviteAllDirect(i);
      fetch::logger.Info("Invited. ");
    }

    fetch::logger.Info("finished inviting. Wait for ", stopCondition_);

    transactionList_.WaitFor(stopCondition_);

    fetch::logger.Info("finished test!\n\n\n");

    finishTimePoint_ = std::chrono::high_resolution_clock::now();
    finished_ = true;

    std::cerr << "Time: " <<  std::chrono::duration_cast<std::chrono::duration<double>>
      (finishTimePoint_ - startTimePoint_).count() << std::endl;
  }

  void ForwardThread()
  {
    std::unique_lock<std::mutex> lock(forwardQueueMutex_);
    lock.unlock();

    while (!destructing_)
    {
      lock.lock();
      forwardQueueCond_.wait(lock);

      while (1)
      {
        fetch::logger.Info("forwarding thread");

        if (forwardQueue_.size() == 0)
        {
          lock.unlock();
          break;
        }

        auto netBlock = forwardQueue_.back();
        forwardQueue_.pop_back();
        lock.unlock();

        auto hash = netBlock.first;

        if (transactionList_.Add(std::move(netBlock)))
        {
          nodeDirectory_.InviteAllForw(transactionList_.Get(hash));
        }

        lock.lock();
      }
    }
  }
};
} // namespace network_benchmark
} // namespace fetch
#endif
