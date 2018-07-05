#ifndef FETCH_CHAINCODE_INTERFACE_HPP
#define FETCH_CHAINCODE_INTERFACE_HPP

#include "core/script/variant.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/identifier.hpp"
#include "ledger/state_database_interface.hpp"

#include <functional>
#include <atomic>
#include <string>
#include <unordered_map>
#include <stdexcept>

namespace fetch {
namespace script { class Variant; }
namespace ledger {

class Contract {
public:

  enum class Status {
    OK = 0,
    FAILED,
    NOT_FOUND,
  };

  using transaction_type = chain::Transaction;
  using query_type = script::Variant;
  using transaction_handler_type = std::function<Status (transaction_type const &)>;
  using transaction_handler_map_type = std::unordered_map<std::string, transaction_handler_type>;
  using query_handler_type = std::function<Status (query_type const &, query_type &)>;
  using query_handler_map_type = std::unordered_map<std::string, query_handler_type>;
  using counter_type = std::atomic<std::size_t>;
  using counter_map_type = std::unordered_map<std::string, counter_type>;
  using state_type = StateDatabaseInterface;

#if 0
  template <typename T>
  class TransactionHandler {
  public:
    using class_type = T;
    using member_function_type = Status (class_type::*)(transaction_type const&);

    TransactionHandler(class_type *instance, member_function_type func)
      : instance_{instance}
      , function_{func} {
    }

    Status operator()(transaction_type const &tx) {
      return (instance_->*function_)(tx);
    }

  private:

    class_type *instance_;
    member_function_type function_;
  };

  template <typename T>
  class QueryHandler {
  public:
    using class_type = T;
    using member_function_type = Status (class_type::*)(query_type const&, query_type &);

    QueryHandler(class_type* instance, member_function_type func)
      : instance_{instance}
      , function_{func} {
    }

    Status operator()(query_type const &query, query_type &response) {
      return (instance_->*function_)(query, response);
    }

  private:

    class_type *instance_;
    member_function_type function_;
  };
#endif

  Contract(Contract const &) = delete;
  Contract(Contract &&) = delete;
  Contract &operator=(Contract const &) = delete;
  Contract &operator=(Contract &&) = delete;

  Status DispatchQuery(std::string const &name, query_type const &query, query_type &response) {
    Status status{Status::NOT_FOUND};

    auto it = query_handlers_.find(name);
    if (it != query_handlers_.end()) {
      status = it->second(query, response);
      ++query_counters_[name];
    }

    return status;
  }

  Status DispatchTransaction(std::string const &name, transaction_type const &tx) {
    Status status{Status::NOT_FOUND};

    auto it = transaction_handlers_.find(name);
    if (it != transaction_handlers_.end()) {
      status = it->second(tx);
      ++transaction_counters_[name];
    }

    return status;
  }

  void Attach(state_type &state) {
    state_ = &state;
  }

  void Detach() {
    state_ = nullptr;
  }

  state_type &state() {
    detailed_assert(state_ != nullptr);
    return *state_;
  }

  std::size_t GetQueryCounter(std::string const &name) {
    auto it = query_counters_.find(name);
    if (it != query_counters_.end()) {
      return it->second;
    } else {
      return 0;
    }
  }

  std::size_t GetTransactionCounter(std::string const &name) {
    auto it = transaction_counters_.find(name);
    if (it != transaction_counters_.end()) {
      return it->second;
    } else {
      return 0;
    }
  }

  bool ParseAsJson(transaction_type const &tx, script::Variant &output);

protected:

  explicit Contract(std::string const &identifer)
    : contract_identifier_{identifer} {
  }

  template <typename C>
  void OnTransaction(std::string const &name, C *instance, Status (C::*func)(transaction_type const&)) {
    if (transaction_handlers_.find(name) == transaction_handlers_.end()) {
      transaction_handlers_[name] = [instance, func](transaction_type const &tx) {
        return (instance->*func)(tx);
      };
      transaction_counters_[name] = 0;
    } else {
      throw std::logic_error("Duplicate transaction handler registered");
    }
  }

  template <typename C>
  void OnQuery(std::string const &name, C *instance, Status (C::*func)(query_type const &, query_type &)) {
    if (query_handlers_.find(name) == query_handlers_.end()) {
      query_handlers_[name] = [instance, func](query_type const &query, query_type &response) {
        return (instance->*func)(query, response);
      };
      query_counters_[name] = 0;
    } else {
      throw std::logic_error("Duplicate query handler registered");
    }
  }

  byte_array::ByteArray CreateStateIndex(byte_array::ByteArray const &suffix) {
    byte_array::ByteArray index(contract_identifier_.name_space());
    index = index + ".state." + suffix;
    return index;
  }

  template <typename T>
  bool GetOrCreateStateRecord(T& record, byte_array::ByteArray const &address) {

    // create the index that is required
    auto index = CreateStateIndex(address);

    // retrieve the state data
    auto document = state().GetOrCreate(index);
    if (document.failed) {
      return false;
    }

    // update the document if it wasn't created
    if (!document.was_created) {
      serializers::ByteArrayBuffer buffer(document.document);
      buffer >> record;
    }

    return true;
  }

  template <typename T>
  bool GetStateRecord(T& record, byte_array::ByteArray const &address) {

    // create the index that is required
    auto index = CreateStateIndex(address);

    // retrieve the state data
    auto document = state().GetOrCreate(index);
    if (document.failed) {
      return false;
    }

    // update the document if it wasn't created
    serializers::ByteArrayBuffer buffer(document.document);
    buffer >> record;

    return true;
  }

  template <typename T>
  void SetStateRecord(T const &record, byte_array::ByteArray const &address) {
    auto index = CreateStateIndex(address);

    // serialize the record to the buffer
    serializers::ByteArrayBuffer buffer;
    buffer << record;

    // store the buffer
    state().Set(index, buffer.data());
  }

private:

  Identifier contract_identifier_;
  query_handler_map_type query_handlers_{};
  transaction_handler_map_type transaction_handlers_{};
  counter_map_type transaction_counters_{};
  counter_map_type query_counters_{};

  state_type *state_{nullptr};
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_CHAINCODE_INTERFACE_HPP
