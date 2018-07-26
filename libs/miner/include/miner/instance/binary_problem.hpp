#pragma once
#include <memory/rectangular_array.hpp>
#include <memory/shared_array.hpp>
#include <unordered_set>
#include <vector>
namespace fetch {
namespace optimisers {
class BinaryProblem {
public:
  typedef double cost_type;
  void Reset() 
  {
    for (std::size_t i = 0; i < couplings_.size(); ++i) couplings_[i] = 0.;
    for (std::size_t i = 0; i < coupling_sum_.size(); ++i) coupling_sum_[i] = 0;

    for(auto &c: couples_to_) c.clear();
    
    energy_offset_ = 0;
    max_abs_coupling_ = 0.0;
    normalisation_constant_ = 1.0;    
  }
  

  
  void Resize(std::size_t const &n,
              std::size_t const &max_connectivity = std::size_t(-1)) {
    couplings_.Resize(n, n);
    coupling_sum_ = memory::SharedArray<cost_type>(n);


    couples_to_.resize(n);

    size_ = n;

    
    Reset();    
  }

  bool Insert(std::size_t const &i, std::size_t const &j, cost_type c) {
    std::size_t A = i;
    std::size_t B = j;
    if (j < i) {
      A = j;
      B = i;
    }


    if( couples_to_[A].find(B) != couples_to_[A].end() ) {
      return false;      
    }
    if( couplings_(A,B) != 0. ) {
      return false;      
    }
    
    couplings_.Set(A, B, c);    
      
    
    if (A != B) {
      couples_to_[A].insert(B);
      couples_to_[B].insert(A);      
      
      coupling_sum_[A] += c;
      coupling_sum_[B] += c;
            
      energy_offset_ += c * 0.25;  // One fourth due to symmetry

      if(c < 0) c = -c;
      if(max_abs_coupling_ < (0.25*c)) max_abs_coupling_ = 0.25*c;
    } else {
      energy_offset_ += c / 2.;
    }
    
    return true;    
  }

  template <typename T>
  void ProgramSpinGlassSolver(T &annealer, bool normalise = true) {
    std::size_t max_conn = MaxConnectivity();
    annealer.Resize(size_,  max_conn );


    for (std::size_t i = 0; i < size_; ++i) {
      cost_type field = -0.5 * (couplings_(i, i) + 0.5 * coupling_sum_[i]);
      cost_type ff = field < 0 ? -field : field;
      if(ff > max_abs_coupling_)  max_abs_coupling_ = ff;
    }
    normalisation_constant_ = 1./max_abs_coupling_ / max_conn;    

    if(!normalise)
      normalisation_constant_ = 1.0;


    
    for (std::size_t i = 0; i < size_; ++i) {
      cost_type field = -0.5 * (couplings_(i, i) + 0.5 * coupling_sum_[i]);

      annealer.Insert(i, i, normalisation_constant_ * field);

      for (std::size_t j = i + 1; j < size_; ++j) {
        if (couplings_(i, j) != 0) {
          annealer.Insert(i, j, normalisation_constant_ * 0.25 * couplings_(i, j));
        }
      }
    }
  }

  std::size_t MaxConnectivity() const
  {
    std::size_t max_connectivity = 0;

    for (auto const &cc : couples_to_) {
      if (cc.size() > max_connectivity) {
        max_connectivity = cc.size();
      }
    }
    return max_connectivity ;    
  }
  
  
  cost_type energy_offset() const { return energy_offset_; }
  memory::RectangularArray<cost_type> const &couplings() const { return couplings_; }
  std::size_t const& size() const 
  {
    return size_;
  }

  cost_type max_abs_coupling() const 
  {
    return max_abs_coupling_; 
  }
  
  cost_type normalisation_constant() const 
  {
    return normalisation_constant_; 
  }
  
  
 private:
  std::size_t size_ = 0;
  cost_type energy_offset_ = 0;

  cost_type max_abs_coupling_ = 0.0;
  cost_type normalisation_constant_ = 1.0;
  
  std::vector<std::unordered_set<uint64_t> > couples_to_;
  memory::RectangularArray<cost_type> couplings_;
  memory::SharedArray<cost_type> coupling_sum_;
};
}
}

