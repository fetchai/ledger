#pragma once
#include "math/ndarray.hpp"

class NDArrayIterator
{
public:

  NDArrayIterator(NDArray &array,
    array_(array),    
    std::vector<std::size_t> const &from,
    std::vector<std::size_t> const &to,
    std::vector<std::size_t> const &step)
    :
    from_(from),
    to_(to),
    step_(step),
    index_(from)    
  {
    Initialise();
  }
  
  NDArrayIterator(NDArray &array,
    std::vector<std::size_t> const &from,
    std::vector<std::size_t> const &to,
    std::vector<std::size_t> const &step,
    std::vector<std::size_t> const &index)
    :
    array_(array),
    from_(from),
    to_(to),
    step_(step),
    index_(index) 
  {
    Initialise();
  }
  
  bool operator++() 
  {
    std::size_t i = 0;
    index_[0] += step_[0];

    std::size_t full_step = step_[0];   
    position_ += full_step;

    while( (i < to_.size()) && (index_[i] >= to_[i] )) {
      position_ -= (index_[i-1]-from_[i]);
      index_[i-1] = from_[i];      
      ++i;
      
      index_[i] += step_[i];
      
      position_ += full_step;      
    } 
  }
  
  bool operator==(NDArrayIterator const &other) const
  {
    assert(from_.size() == other.from_.size());
    
    bool ret = true;
    for(std::size_t i=0; i <from_.size(); ++i) {
      ret &= (other.index_[i] == index_[i]);
    }

    return ret;
  }
  
  
  std::size_t size() const 
  {
    return size_;
  }
  
private:

  void Initialise() 
  {
    size_ = 1;
    offsets_.resize(from_.size());
    position_ = 1;
    
    for(std::size_t i = 0; i < from_.size(); ++i)
    {
      std::size_t elements = (to_[i] - from_[i]) / step_[i] + 1;
      size_ *= elements;
      
      position_ *= array_.shape()[i];
    }
  }

  NDArray &array_;
  
  std::vector<std::size_t> from_;
  std::vector<std::size_t> to_;
  std::vector<std::size_t> step_;

  std::vector<std::size_t> index_;
  std::size_t position_;
  
  std::size_t size_ = 0;

  
};
