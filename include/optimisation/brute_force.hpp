#ifndef OPTIMISATION_BRUTE_FORCE_HPP
#define OPTIMISATION_BRUTE_FORCE_HPP

#include <memory/shared_array.hpp>
#include <memory/rectangular_array.hpp>

#include <limits>

namespace fetch {
namespace optimisation {

template <typename T>
class BruteForceOptimiser {
 public:
  typedef T cost_type;
  typedef uint64_t state_type;

  BruteForceOptimiser(std::size_t const &n)
      : couplings_(n, n),
        cache_(1ull << n),
        variables_(n),
        total_state_count_(1ull << n),
        size_(n) {
    for (std::size_t i = 0; i < n; ++i)
      for (std::size_t j = 0; j < n; ++j) {
        couplings_(i, j) = 0;
      }
  }

  cost_type const &operator()(std::size_t const &i,
                              std::size_t const &j) const {
    return couplings_(std::min(i, j), std::max(i, j));
  }

  cost_type &operator()(std::size_t const &i, std::size_t const &j) {
    return couplings_(std::min(i, j), std::max(i, j));
  }

  cost_type FindMinimum() {
    state_type ret;
    return FindMinimum(ret);
  }

  cost_type FindMinimum(state_type &ret) {
    /*
      TODO: update implementation
    UpdateCouplingCache(); // FIXME: Only call if needed!
    cost_type best = std::numeric_limits< cost_type >::max();
    for(std::size_t i=0; i < total_state_count_ ; ++i) {
      // FIXME add local fields
      cost_type test = cache_[i];
      for(std::size_t k=0; k < pos ; ++k)
        change += couplings_(k,pos)  * VariableAsSpin(i,k);

      for(std::size_t k=pos+1; k < variables_ ; ++k)
        change += couplings_(pos, k)  * VariableAsSpin(i,k);

      if(test < best) {
        best = test;
        ret = i;
      }
    }

    return best;
    */
    return 0;
  }

  void UpdateCouplingCache() {
    cost_type cost = 0;
    for (std::size_t i = 0; i < couplings_.height(); ++i)
      for (std::size_t j = i; j < couplings_.width(); ++j)
        cost += couplings_(i, j);
    cache_[0] = cost;

    std::size_t pos = 0;
    std::size_t bit_at = 1;
    state_type mask = bit_at - 1;

    for (state_type i = 1; i < total_state_count_;
         ++i) {  // FIXME: add Z2 symmetry
      if ((i & bit_at) == 0) {
        bit_at <<= 1;
        mask = bit_at - 1;
        ++pos;
      }

      state_type j = (i & mask);

      cost = cache_[j];
      cost_type change = 0;
      // FIXME: use cache table to speed up
      // Speeding this up could give as much as a factor of 8
      for (std::size_t k = 0; k < pos; ++k)
        change += couplings_(k, pos) * VariableAsSpin(i, k);

      for (std::size_t k = pos + 1; k < variables_; ++k)
        change += couplings_(pos, k) * VariableAsSpin(i, k);

      cost -= 2 * change;
      cache_[i] = cost;
    }
  }

  cost_type CostOf(state_type const &c) {
    cost_type ret = 0;
    for (std::size_t i = 0; i < variables_; ++i) {
      int s1 = VariableAsSpin(c, i);
      ret += s1 * couplings_(i, i);
      for (std::size_t j = i + 1; j < variables_; ++j) {
        int s2 = VariableAsSpin(c, j);
        ret += s1 * s2 * couplings_(i, j);
      }
    }
    return ret;
  }

  std::size_t const &size() const { return size_; }

 private:
  /*
     Spin convention: 0 -> spin up, 1
                      1 -> spin down, -1
   */
  static int VariableAsSpin(state_type const &i, std::size_t k) {
    return (1 - 2 * ((int(i) >> k) & 1));
  }

  memory::RectangularArray<cost_type> couplings_;
  memory::SharedArray<cost_type> cache_;
  uint64_t variables_ = 0;
  uint64_t total_state_count_ = 0;
  std::size_t size_ = 0;
};
};
};

#endif
