#ifndef VECTORIZE_ITERATOR_HPP
#define VECTORIZE_ITERATOR_HPP
#include<cassert>

namespace fetch {
namespace vectorize {

template <typename T, std::size_t N = sizeof(T), typename S = typename VectorInfo<T,N>::register_type  >
class VectorRegisterIterator {
public:
  typedef T type;
  typedef VectorRegister< T, N, S > vector_register_type;
  typedef typename vector_register_type::mm_register_type mm_register_type;

  /*
  VectorRegisterIterator(type const *d)
    : ptr_( (mm_register_type*)d ),
      end_(nullptr) { }
  */  
  VectorRegisterIterator(type const *d, std::size_t size)
    : ptr_( (mm_register_type*)d ),
      end_((mm_register_type*)(d+size)) { }
  
  void Next(vector_register_type &m) {
    assert( (end_==nullptr) || (ptr_ < end_));
    
    m.data() = *ptr_;
    ++ptr_;
  }
        
private:
  mm_register_type *ptr_;
  mm_register_type *end_;
};

};
};

#endif
