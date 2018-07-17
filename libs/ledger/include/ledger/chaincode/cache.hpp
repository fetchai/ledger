#ifndef FETCH_CACHE_HPP
#define FETCH_CACHE_HPP

#include "core/meta/is_log2.hpp"
#include "ledger/chaincode/factory.hpp"

#include <unordered_map>
#include <algorithm>
#include <chrono>

namespace fetch {
namespace ledger {

class ChainCodeCache {
public:
  using chain_code_type = ChainCodeFactory::chain_code_type;
  using clock_type = std::chrono::high_resolution_clock;
  using timepoint_type = clock_type::time_point;

  static constexpr uint64_t CLEANUP_PERIOD = 16;
  static constexpr uint64_t CLEANUP_MASK = CLEANUP_PERIOD - 1u;
  static constexpr auto CACHE_LIFETIME = std::chrono::hours{1};

  struct Element {
    Element(chain_code_type c) : chain_code{c} {}

    chain_code_type chain_code;
    timepoint_type  timestamp{clock_type::now()};
  };

  using underlying_cache_type = std::unordered_map<std::string, Element>;
  using cache_value_type = underlying_cache_type::value_type;

  chain_code_type Lookup(std::string const &contract_name) {

    // attempt to locate the contract in the cache
    chain_code_type contract = FindInCache(contract_name);

    // if this fails create the contract
    if (!contract) {
      contract = CreateContract(contract_name);
    }

    // periodically run cache maintainance
    if ((++counter_ & CLEANUP_MASK) == 0) {
      RunMaintenance();
    }

    return contract;
  }

  ChainCodeFactory const &factory() const {
    return factory_;
  }

private:

  chain_code_type FindInCache(std::string const &name) {
    chain_code_type contract;

    // attempt to lookup the contract in the cache
    auto it = cache_.find(name);
    if (it != cache_.end()) {

      // extract the contract and refresh the cache timestamp
      contract = it->second.chain_code;
      it->second.timestamp = clock_type::now();
    }

    return contract;
  }

  chain_code_type CreateContract(std::string const &name) {
    chain_code_type contract = factory_.Create(name);

    // update the cache
    cache_.emplace(name, contract);

    return contract;
  }

  void RunMaintenance() {
    timepoint_type const now = clock_type::now();

    for (auto it = cache_.begin(), end = cache_.end(); it != end;) {
      auto const delta = now - it->second.timestamp;
      if (delta >= decltype(CACHE_LIFETIME){CACHE_LIFETIME}) {
        it = cache_.erase(it);
      } else {
        ++it;
      }
    }
  }

  std::size_t counter_;
  underlying_cache_type cache_;
  ChainCodeFactory factory_;

  static_assert(meta::IsLog2<CLEANUP_PERIOD>::value, "Clean up period must be a valid power of 2");
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_CACHE_HPP
