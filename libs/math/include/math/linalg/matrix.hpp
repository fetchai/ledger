#ifndef MATH_LINALG_MATRIX_HPP
#define MATH_LINALG_MATRIX_HPP
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "vectorise/threading/pool.hpp"
#include "math/rectangular_array.hpp"

#include <iostream>
#include <limits>
#include <vector>

namespace fetch {
namespace math {
namespace linalg {

template <typename T, typename C = fetch::memory::SharedArray<T>,
  typename S = fetch::math::RectangularArray<T, C> >
class Matrix : public S {
public:
  typedef S super_type;
  typedef typename super_type::type type;
  typedef typename super_type::vector_register_type vector_register_type;
  typedef typename super_type::vector_register_iterator_type vector_register_iterator_type;
  typedef RectangularArray<T, C, true, true> working_memory_2d_type;
  
  typedef Matrix< T, C, S > self_type; 

  
  enum {
    E_SIMD_BLOCKS = super_type::container_type::E_SIMD_COUNT
  };

  Matrix() = default;
  Matrix(Matrix &&other) = default;
  Matrix(Matrix const &other) = default;
  Matrix &operator=(Matrix const &other) = default;
  Matrix &operator=(Matrix &&other) = default;

  Matrix(super_type const &other) : super_type(other) {}
  Matrix(super_type &&other) : super_type(std::move(other)) {}
  Matrix(byte_array::ConstByteArray const &c) {
    std::size_t n = 1;
    std::vector<type> elems;
    elems.reserve(1024);
    bool failed = false;

    for (uint64_t i = 0; i < c.size();) {
      uint64_t last = i;
      switch (c[i]) {
      case ';':
        ++n;
        ++i;
        break;
      case ',':
      case ' ':
      case '\n':
      case '\t':
      case '\r':
        ++i;
        break;
      default:
        if (byte_array::consumers::NumberConsumer<1, 2>(c, i) == -1) {
          failed = true;
        } else {
          elems.push_back(type(atof(c.char_pointer() + last)));
        }
        break;
      }
    }
    std::size_t m = elems.size() / n;

    if ((m * n) != elems.size()) {
      failed = true;
    }

    if (!failed) {
      this->Resize(n, m);
      this->SetAllZero();

      std::size_t k = 0;
      for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < m; ++j) {
          this->Set(i, j, elems[k++]);
        }
      }
    }
  }

  Matrix(std::size_t const &h, std::size_t const &w) : super_type(h, w) {}

  static Matrix Zeros(std::size_t const &n, std::size_t const &m) {
    Matrix ret;
    ret.LazyResize(n, m);
    ret.data().SetAllZero();
    return ret;
  }



  static Matrix UniformRandom(std::size_t const &n, std::size_t const &m) {
    Matrix ret;

    ret.LazyResize(n, m);
    ret.FillUniformRandom();
    
    ret.SetPaddedZero();

    return ret;
  }


  
  template<typename G >
  self_type &Transpose(G const &other) {
    this->LazyResize(other.width(), other.height());

    for (std::size_t i = 0; i < other.height(); ++i)
      for (std::size_t j = 0; j < other.width(); ++j)
        this->At(j, i) = other.At(i, j);

    return *this;
  }

  template <typename F>
  void SetAll(F &val) {
    for (auto &a : *this) {
      a = val;
    }
  }

  /* Lazy dot routine to compute C = Dot(A, B).
   * @A is the first matrix.
   * @B is the second matrix.
   *
   * This routine takes care of the working memory. 
   */
  Matrix &Dot(Matrix const &A, Matrix const &B) 
  {
    working_memory_2d_type tmpA;
    working_memory_2d_type tmpB;
    tmpA.LazyResize( A.height(), A.width() );
    tmpB.LazyResize( B.width() , B.height());    
    return Dot(A, B, tmpA, tmpB);      
  }

  
  /* Dot routine to compute C = Dot(A, B).
   * @A is the first matrix.
   * @B is the second matrix.
   * @tmpA working memory.
   * @tmpB working memory.
   */    
  Matrix &Dot(Matrix const &A, Matrix const &B,
    working_memory_2d_type &tmpA, working_memory_2d_type &tmpB) {
    // Assumptions made by the routine
    assert(A.height() == tmpA.height());
    assert(A.width() == tmpA.width());

    assert(B.width() == tmpB.height());
    assert(B.height() == tmpB.width());

    assert(A.height() == B.width());    
    
    // 
    tmpA.Copy( A );

    for (std::size_t i = 0; i < B.height(); ++i)
      for (std::size_t j = 0; j < B.width(); ++j)
        tmpB.At(j, i) = B.At(i, j);
      
    tmpA.SetPaddedZero();
    tmpB.SetPaddedZero();
      
    return DotTransposedOf(tmpA, tmpB);
  }
private:
  Matrix &DotTransposedOf(working_memory_2d_type   const &mA,
    working_memory_2d_type const &mB) {    

    assert(mA.width() == mB.width()); 
    assert(this->height() == mA.height());
    assert(this->width() == mB.height());

    threading::Pool pool;    
    std::size_t  width = mA.padded_width(); 
    for (std::size_t i = 0; i < mA.height(); ++i) {

      pool.Dispatch( [i, this, mA, mB, width]() {
          
          vector_register_type a, b;
          vector_register_iterator_type ib(mB.data().pointer(), mB.data().size());
          
          for (std::size_t j = 0; j < mB.height(); ++j) {
            std::size_t k = 0;
            type ele = 0;
            
            vector_register_iterator_type ia(
              mA.data().pointer() + i * mA.padded_width(), mA.padded_height());
            vector_register_type c(type(0));
            
            for (; k < width; k += vector_register_type::E_BLOCK_COUNT) {
              ia.Next(a);
              ib.Next(b);
              c = c + a * b;
            }
            
            for (std::size_t l = 0; l < vector_register_type::E_BLOCK_COUNT; ++l) {
              ele += first_element(c);
              c = shift_elements_right(c);
            }
            
            this->Set(i, j, ele);
          }
        });
      
    }

    pool.Wait();

    
    return *this;
  }

};
}
}
}
#endif


