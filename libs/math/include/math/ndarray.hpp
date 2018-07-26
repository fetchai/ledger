#pragma once
#include"vectorise/memory/array.hpp"
#include"math/shape_less_array.hpp"


#include<vector>
#include<utility>

namespace fetch {
namespace math {
    
template< typename T, typename C = SharedArray<T> >
class NDArray : public ShapeLessArray< T, C > {
public:
  typedef T type;
  typedef C container_type;
  typedef typename container_type::vector_register_type vector_register_type;
  typedef ShapeLessArray< T, C > super_type;
  typedef NDArray<T, C> self_type;            
      
  NDArray(std::size_t const &n = 0) : super_type(n) {
    size_ = 0;        
  }
           
  void Copy(self_type &copy) {
    copy.data_ = this->data_.Copy();
    copy.size = this->size_;
  }      

  void Flatten() {
    shape_.clear();
    shape_.push_back( super_type::size() );
  }

  void LazyReshape(std::vector< std::size_t > const &shape) {
    shape_ = shape;
  }

  template< typename... Indices>
  type &operator()(Indices const &...indices) 
  {
    assert( sizeof...(indices) <= shape_.size() );
    std::size_t index = 0, shift = 1;
    ComputeIndex(0, index, shift, indices ... );
    return this->operator[](index); 
  }


  template< typename D, typename... Indices>      
  void Get( D& dest, Indices const &...indices ) 
  {
    std::size_t shift = 1, size;
    int dest_rank = int(shape_.size()) - int( sizeof...(indices));
        
    assert( dest_rank > 0 );

    std::size_t rank_offset = (shape_.size() - std::size_t(dest_rank));        
        
    for(std::size_t i= rank_offset; i < shape_.size(); ++i  )
    {
      shift *= shape_[i];      
    }
        
    size = shift;        

    std::size_t offset = 0;        
    ComputeIndex( 0, offset, shift, indices... );
      
    dest.Resize( shape_, rank_offset );
    assert( size <= dest.size());


    for (std::size_t i = 0; i < size; ++i) {
      dest[i] = this->operator[](offset + i );
    }
  }
      
  void Reshape(std::vector< std::size_t > const &shape) {
    std::size_t total = 1;
    shape_.reserve( shape.size() );
    shape_.clear();        
        
    for(auto const &s : shape) {
      total *= s;
      shape_.push_back(s);
    }

    assert(total <= this->size());
        
    size_ = total;
  }
            
      
  std::vector< std::size_t > const &shape() const {
    return shape_;
  }
      

private:
  template< typename... Indices>
  void ComputeIndex(std::size_t const& N, std::size_t &index, std::size_t &shift, std::size_t const &next, Indices const &...indices) const
  {
    ComputeIndex(N + 1, index, shift, indices ... );
        
    assert( N < shape_.size());
    index += next * shift;        
    shift *= shape_[N];        
  }

  template< typename... Indices>
  void ComputeIndex(std::size_t const& N, std::size_t &index, std::size_t &shift, int const &next, Indices const &...indices) const
  {
    ComputeIndex(N + 1, index, shift, indices ... );
        
    assert( N < shape_.size());
    index += std::size_t(next) * shift;        
    shift *= shape_[N];        
  }
      
  void ComputeIndex(std::size_t const& N, std::size_t &index, std::size_t &shift, std::size_t const &next ) const
  {
    index += next * shift;        
    shift *= shape_[N];
  }

  void ComputeIndex(std::size_t const& N, std::size_t &index, std::size_t &shift, int const &next ) const
  {
    index += std::size_t(next) * shift;        
    shift *= shape_[N];
  }
      
  std::size_t size_ = 0;      
  std::vector< std::size_t > shape_;

};
}
}

