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

#include"core/byte_array/const_byte_array.hpp"
#include"core/random/lfg.hpp"
#include"core/logger.hpp"
#include"ledger/chain/transaction.hpp"
#include"./network_classes.hpp"
#include"./node_directory.hpp"
#include"./transaction_list.hpp"
#include"../tests/include/helper_functions.hpp"

namespace fetch
{
namespace network_benchmark
{

typedef std::chrono::high_resolution_clock::time_point time_point;

class NodeBasic
{
public:
  explicit NodeBasic(network::NetworkManager tm) :
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

  void transactionsPerCall(uint64_t tpc)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<std::mutex> mlock(mutex_);
    transactionsPerCall_ = tpc;
    fetch::logger.Info("set transactions per call to ", tpc);
  }

  void TransactionsToSync(uint64_t transactionsToSync)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<std::mutex> mlock(mutex_);
    fetch::logger.Info("set transactions to sync to ", transactionsToSync);
    fetch::logger.Info("Building...");
    PrecreateTrans(transactionsToSync);
    AddTransToList();
  }

  void stopCondition(uint64_t stopCondition)
  {
    LOG_STACK_TRACE_POINT;
    stopCondition_ = stopCondition;
  }

  void isSlave()
  {
    slave_ = true;
  }

  void StartTime(uint64_t startTime)
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

  // TODO: (`HUT`) : get rid of start in fn names
  void StartTestAsMaster(uint64_t startTime)
  {
    if (thread_.joinable())
    {
      thread_.join();
    }

    thread_   = std::thread([this]() { TestAsMaster();});
  }

  double TimeToComplete()
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
    slave_ = false;
    sendIndex_ = 0;
  }

  bool finished() const
  {
    std::cerr << "Trans list: " << transactionList_.size()
      << " of " << stopCondition_ << std::endl;
    return finished_;
  }

  void TransactionSize(uint32_t transactionSize)
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

  void PushConfident(block_hash const & blockHash, block_type &block)
  {
    transactionList_.Add(blockHash, std::move(block));
    auto cb = [&]{ nodeDirectory_.InviteAllForw(blockHash, transactionList_.Get(blockHash));};
    std::async(std::launch::async, cb);
  }

  inline std::size_t GetNextIndex()
  {
    std::unique_lock<std::mutex> lock(forwardQueueMutex_);
    static std::size_t index= 0;
    return index++;
  }

  inline void IndexIsSafe(std::size_t index)
  {
    forwardQueueSafe_[index] = 5;
    std::unique_lock<std::mutex> lock(forwardQueueMutex_);
    wakeMe = true;
    forwardQueueCond_.notify_one();
  }

  void Push(block_hash const & blockHash, block_type &block)
  {
    std::size_t thisIndex = GetNextIndex();

    forwardQueueHash_[thisIndex] = blockHash;
    forwardQueue_[thisIndex] = std::move(block);

    IndexIsSafe(thisIndex);
  }

  bool SendNext()
  {
    std::size_t sendIndex = sendIndex_++;
    std::cerr << "Sending: " << sendIndex << std::endl;
    if(sendIndex >= premadeTrans_.size())
    {
      return false;
    }

    network_block &transBlock = premadeTrans_[sendIndex];
    nodeDirectory_.InviteAllBlocking(transBlock.first, transBlock.second);
    return true;
  }

  int ping()
  {
    return 4;
  }

  ///////////////////////////////////////////////////////////
  // HTTP functions to check that synchronisation was successful
  std::set<transaction_type> GetTransactions()
  {
    LOG_STACK_TRACE_POINT;
    return transactionList_.GetTransactions();
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    LOG_STACK_TRACE_POINT;
    return transactionList_.TransactionsHash();
  }

private:
  NodeDirectory                           nodeDirectory_;   // Manage connections to other nodes
  TransactionList<block_hash, block_type> transactionList_; // List of all transactions
  fetch::mutex::Mutex                     mutex_;

  // Transmitting thread
  std::thread                thread_;
  uint64_t                   transactionsPerCall_ = 1000;
  uint32_t                   txPad_               = 0;
  std::vector<network_block> premadeTrans_;
  uint64_t                   stopCondition_{0};
  uint64_t                   startTime_{0};
  time_point                 startTimePoint_      = std::chrono::high_resolution_clock::now();
  time_point                 finishTimePoint_     = std::chrono::high_resolution_clock::now();
  bool                       finished_            = false;
  bool                       slave_               = false;
  bool                       destructing_         = false;
  std::atomic<std::size_t>   sendIndex_{0};

  mutable std::condition_variable forwardQueueCond_;
  std::mutex                      forwardQueueMutex_;
  bool                            wakeMe{false};
  std::thread                     forwardQueueThread_{[this]() { ForwardThread();}};
  std::array<block_type, 10000>   forwardQueue_;
  std::array<block_hash, 10000>   forwardQueueHash_;
  std::array<int, 10000>          forwardQueueSafe_;

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

    for (uint64_t i = 0; i < blocks; ++i)
    {
      network_block &transBlock = premadeTrans_[i];
      transBlock.second.resize(transactionsPerCall_);

      for (std::size_t j = 0; j < transactionsPerCall_; j++)
      {
        transBlock.second[j] = common::NextTransaction<transaction_type>(txPad_);
      }

      // Use the first Tx for the block hash, should be adequate to avoid collisions (sha256)
      transBlock.first = common::Hash(transBlock.second[0].summary().transaction_hash);
    }
  }

  void AddTransToList()
  {
    for (auto &netBlock : premadeTrans_)
    {
      transactionList_.Add(netBlock.first, netBlock.second);
    }
  }

  ///////////////////////////////////////////////////////////
  // Threads
  void SendTransactions()
  {
    LOG_STACK_TRACE_POINT;
    finished_ = false;
    common::BlockUntilTime(startTime_);
    startTimePoint_ = std::chrono::high_resolution_clock::now();

    if(!slave_)
    {
      for (auto &i : premadeTrans_)
      {
        //transactionList_.Add(i); // No need since this has been done pre-test
        nodeDirectory_.InviteAllDirect(i.first, i.second);
      }
    }

    transactionList_.WaitFor(stopCondition_);

    finishTimePoint_ = std::chrono::high_resolution_clock::now();
    finished_ = true;

    std::cerr << "Time: " <<  std::chrono::duration_cast<std::chrono::duration<double>>
      (finishTimePoint_ - startTimePoint_).count() << std::endl;
  }

  // Thread for forwarding incoming transaction blocks
  void ForwardThread()
  {
    forwardQueueSafe_.fill(0);

    static std::size_t queueIndex = 0;
    while (!destructing_)
    {
      std::unique_lock<std::mutex> lock(forwardQueueMutex_);
      forwardQueueCond_.wait(lock, [this] {return wakeMe;});
      wakeMe = false;
      lock.unlock();

      while (forwardQueueSafe_[queueIndex] == 5)
      {
        auto hash             = forwardQueueHash_[queueIndex];
        auto &netBlock        = forwardQueue_[queueIndex++];

        if (transactionList_.Add(hash, std::move(netBlock)))
        {
          nodeDirectory_.InviteAllForw(hash, transactionList_.Get(hash));
        }
      }
    }
  }

  // Control all the other nodes to select the order they transmit blocks
  void TestAsMaster()
  {
    LOG_STACK_TRACE_POINT;
    finished_ = false;
    common::BlockUntilTime(startTime_);
    startTimePoint_ = std::chrono::high_resolution_clock::now();

    nodeDirectory_.ControlSlaves();

    finishTimePoint_ = std::chrono::high_resolution_clock::now();
    finished_ = true;

    std::cerr << "Time: " <<  std::chrono::duration_cast<std::chrono::duration<double>>
      (finishTimePoint_ - startTimePoint_).count() << std::endl;
  }

};
} // namespace network_benchmark
} // namespace fetch
#endif
