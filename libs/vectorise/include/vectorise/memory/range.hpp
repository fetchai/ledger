#pragma once


namespace fetch {
namespace memory {

class TrivialRange {
public:
  typedef std::size_t size_type;
      
  TrivialRange(size_type const &from, size_type const &to)
    : from_(from), to_(to) { }

  size_type const &from() const { return from_; }
  size_type const &to() const { return to_; }
  size_type step() const 
  {
    return 1;
  }
  

  template<std::size_t S>
  size_type SIMDFromLower() const {
    size_type G = from_ / S;
    return G*S;
  }
  
  template<std::size_t S>
  size_type SIMDFromUpper() const {
    size_type G = from_ / S;
    if(G * S < from_) ++G;   
    return G*S;
  }

  template<std::size_t S>  
  size_type SIMDToLower() const {
    size_type G = to_ / S;
    return G*S;
  }

  template<std::size_t S>  
  size_type SIMDToUpper() const {
    size_type G = to_ / S;
    if(G * S < to_) ++G;       
    return G*S;
  }
  
private:
  size_type from_=0, to_=0;
};


class Range {
public:
  typedef std::size_t size_type;
      
  Range(size_type const &from = 0, size_type const &to = size_type(-1), size_type const &step = 1)
    : from_(from), to_(to), step_(step) { }

  size_type const &from() const { return from_; }
  size_type const &to() const { return to_; }
  size_type const &step() const { return step_; }

  bool is_trivial() const 
  {
    return (step_ == 1);
  }

  bool is_undefined() const 
  {
    return ((from_ == 0 ) && ( to_ == size_type(-1)));
  }
  
  
  TrivialRange ToTrivialRange(size_type const &size) const 
  {
    return TrivialRange(from_, std::min(size,to_));
  }
  
private:
  size_type from_=0, to_=0, step_=1;
};

}
}
