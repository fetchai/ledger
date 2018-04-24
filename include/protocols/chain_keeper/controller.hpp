#ifndef PROTOCOLS_CHAIN_KEEPER_MANAGER_HPP
#define PROTOCOLS_CHAIN_KEEPER_MANAGER_HPP
#include "byte_array/const_byte_array.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "serializer/referenced_byte_array.hpp"

#include "chain/block.hpp"
#include "chain/consensus/proof_of_work.hpp"
#include "chain/transaction.hpp"
#include "protocols/fetch_protocols.hpp"

#include "protocols/chain_keeper/transaction_manager.hpp"

#include "mutex.hpp"
#include "protocols/chain_keeper/commands.hpp"
#include "service/client.hpp"
#include "service/publication_feed.hpp"

#include "logger.hpp"
#include "protocols/swarm/entry_point.hpp"

#include <limits>
#include <map>
#include <set>
#include <stack>
#include <vector>

namespace fetch {
namespace protocols {

class ChainKeeperController : public fetch::service::HasPublicationFeed {
 public:
  // TODO: Get from chain manager
  // Transaction defs
  typedef fetch::chain::TransactionSummary transaction_summary_type;
  typedef fetch::chain::Transaction transaction_type;
  typedef typename transaction_type::digest_type tx_digest_type;

  // Other groups
  typedef fetch::service::ServiceClient<fetch::network::TCPClient> client_type;
  typedef std::shared_ptr<client_type> client_shared_ptr_type;

  ChainKeeperController(uint64_t const &protocol,
                        network::ThreadManager *thread_manager,
                        EntryPoint &details)
      : thread_manager_(thread_manager),
        details_(details),
        block_mutex_(__LINE__, __FILE__),
        chain_keeper_friends_mutex_(__LINE__, __FILE__),
        grouping_parameter_(1) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    fetch::logger.Debug("Entering ", __FUNCTION_NAME__);

    details_.configuration = EntryPoint::NODE_CHAIN_KEEPER;
  }

  // TODO: Change signature to std::vector< EntryPoint >
  EntryPoint Hello(std::string host) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    fetch::logger.Debug("Exchaning group details (RPC reciever)");

    std::lock_guard<fetch::mutex::Mutex> lock(details_mutex_);

    details_.configuration = EntryPoint::NODE_CHAIN_KEEPER;
    if (details_.host != host) {
      details_.host = host;
    }

    return details_;
  }

  std::vector<transaction_type> GetTransactions() {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    return tx_manager_.LastTransactions();
  }

  std::vector<transaction_summary_type> GetSummaries() {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    return tx_manager_.LatestSummaries();
  }

  bool PushTransaction(transaction_type tx) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    if (!tx.UsesGroup(details_.group, grouping_parameter_)) {
      fetch::logger.Debug("Transaction not belonging to group");
      return false;
    }

    block_mutex_.lock();

    tx.UpdateDigest();
    if (!tx_manager_.AddTransaction(tx)) {
      block_mutex_.unlock();
      return false;
    }
    auto const &groups = tx_manager_.Next().groups();
    // TODO: Migrate to shared_ptr< Transaction >
    fetch::logger.Highlight(
        "Total group size: ", tx_manager_.Next().groups().size(), " ",
        groups.size());
    block_mutex_.unlock();

    fetch::logger.Warn("Verify transaction");

    return true;
  }

  void ConnectTo(byte_array::ByteArray const &host, uint16_t const &port) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    // Chained tasks
    client_shared_ptr_type client;
    std::size_t i = 0;
    while ((!client) && (i < 3)) {  // TODO: make configurable

      client = std::make_shared<client_type>(host, port, thread_manager_);
      auto ping_promise =
          client->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::PING);
      if (!ping_promise.Wait(500))  // TODO: Make configurable
      {
        fetch::logger.Debug("Server not repsonding - retrying !");
      }
      ++i;
    }

    if (!client) {
      fetch::logger.Error("Server not repsonding - hanging up!");
      return;
    }

    EntryPoint d;
    d.host = std::string(host);
    d.port = port;
    d.http_port = uint16_t(-1);
    d.group = 0;  // TODO: get and check that it is right
    d.configuration = 0;

    chain_keeper_friends_mutex_.lock();
    chain_keeper_friends_.push_back(client);
    friends_details_.push_back(d);
    chain_keeper_friends_mutex_.unlock();
  }

  void ListenTo(std::vector<EntryPoint> list)  // TODO: Rename
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    fetch::logger.Highlight("Updating connectivity for ", details_.host, ":",
                            details_.port);

    for (auto &e : list) {
      fetch::logger.Highlight("  - ", e.host, ":", e.port, ", group ", e.group);
      details_mutex_.lock();
      bool self = (e.host == details_.host) && (e.port == details_.port);
      bool same_group = (e.group == details_.group);
      details_mutex_.unlock();

      if (self) {
        fetch::logger.Debug("Skipping myself");
        continue;
      }

      if (!same_group) {
        fetch::logger.Debug("Connectiong not belonging to same group");

        continue;
      }

      // TODO: implement max connectivity

      bool found = false;
      chain_keeper_friends_mutex_.lock();
      for (auto &d : friends_details_) {
        if ((d.host == e.host) && (d.port == e.port)) {
          found = true;
          break;
        }
      }

      chain_keeper_friends_mutex_.unlock();
      if (!found) {
        ConnectTo(e.host, e.port);
      }
    }
  }

  void SetGroupNumber(group_type group, group_type total_groups) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    fetch::logger.Debug("Setting group numbers: ", group, " ", total_groups);
    grouping_parameter_ = total_groups;
    details_.group = group;

    block_mutex_.lock();
    tx_manager_.set_group(group);
    block_mutex_.unlock();
  }

  uint16_t count_outgoing_connections() {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    std::lock_guard<fetch::mutex::Mutex> lock(chain_keeper_friends_mutex_);
    return uint16_t(chain_keeper_friends_.size());
  }

  group_type group_number()  // TODO: Change to atomic
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    return details_.group;
  }

  void with_peers_do(std::function<void(std::vector<client_shared_ptr_type>,
                                        std::vector<EntryPoint> const &)>
                         fnc) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    chain_keeper_friends_mutex_.lock();
    fnc(chain_keeper_friends_, friends_details_);
    chain_keeper_friends_mutex_.unlock();
  }

  void with_peers_do(
      std::function<void(std::vector<client_shared_ptr_type>)> fnc) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    chain_keeper_friends_mutex_.lock();
    fnc(chain_keeper_friends_);
    chain_keeper_friends_mutex_.unlock();
  }

  std::size_t unapplied_transaction_count() const {
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    return tx_manager_.unapplied_count();
  }

  std::size_t applied_transaction_count() const {
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    return tx_manager_.applied_count();
  }

  std::size_t transaction_count() const {
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    return tx_manager_.size();
  }

  bool AddBulkTransactions(
      std::unordered_map<tx_digest_type, transaction_type,
                         typename TransactionManager::hasher_type> const
          &new_txs) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    return tx_manager_.AddBulkTransactions(new_txs);
  }

  void with_transactions_do(
      std::function<void(std::vector<transaction_type> &)> fnc) {
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    tx_manager_.with_transactions_do(fnc);
  }

 private:
  network::ThreadManager *thread_manager_;
  EntryPoint &details_;
  mutable fetch::mutex::Mutex details_mutex_;

  mutable fetch::mutex::Mutex block_mutex_;

  std::vector<client_shared_ptr_type> chain_keeper_friends_;
  std::vector<EntryPoint> friends_details_;
  fetch::mutex::Mutex chain_keeper_friends_mutex_;

  std::atomic<group_type> grouping_parameter_;

  TransactionManager tx_manager_;
};
}
}

#endif
