#ifndef MATH_LINALG_MATRIX_HPP
#define MATH_LINALG_MATRIX_HPP

#include "memory/rectangular_array.hpp"
#include "byte_array/const_byte_array.hpp"
#include "byte_array/consumers.hpp"

#include <iostream>
#include <limits>
#include <vector>

namespace fetch {
namespace math {
namespace linalg {

template <typename T, typename C = fetch::memory::SharedArray<T>,
          typename A = fetch::memory::RectangularArray<T, C> >
class Matrix : public A {
 public:
  typedef A super_type;
  typedef typename super_type::type type;
  typedef typename super_type::vector_register_type vector_register_type;
  typedef typename super_type::vector_register_iterator_type vector_register_iterator_type;   
  
  enum {
    INVERSION_OK = 0,
    INVERSION_SINGULAR = 1,
    E_SIMD_BLOCKS = super_type::container_type::E_SIMD_COUNT
  };



  Matrix() = default;
  Matrix(Matrix &&other) = default;
  Matrix(Matrix const &other) = default;
  Matrix &operator=(Matrix const &other) = default;
  Matrix &operator=(Matrix &&other) = default;

  Matrix(super_type const &other) : super_type(other) { }
  Matrix(super_type &&other) : super_type(std::move(other)) {  }
  Matrix(byte_array::ConstByteArray const &c) {
    std::size_t n = 1;
    std::vector< type > elems;
    elems.reserve(1024);
    bool failed = false;
    
    for(uint64_t i=0; i < c.size(); ) {
      uint64_t last = i;
      switch(c[i]) {
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
        if(byte_array::consumers::NumberConsumer<1,2>(c, i) == -1) {
          failed = true;
        } else {
          elems.push_back( atof( c.char_pointer() + last ));
        }
        break;
      }
    }
    std::size_t m = elems.size() / n;
    
    if( (m * n) != elems.size() ) {
      failed = true;
    }
    
    if(!failed) {
      this->Resize(n,m);
      for(std::size_t i = 0; i < elems.size(); ++i) {
        this->At(i) = elems[i];
      }
    }
  }
  
  Matrix(std::size_t const &h, std::size_t const &w) : super_type(h, w) {
    for(std::size_t i=0; i < h; ++i) {
    for(std::size_t j=0; j < w; ++j) {
      this->Set(i,j, type(0));
    }
    }
  }

  Matrix Copy() const { return Matrix( super_type::Copy() ); }
  
#define FETCH_ADD_OPERATOR(OP,VOP)                                      \
  Matrix &operator OP(Matrix const &other) {                            \
  assert(other.size() == this->size());                                 \
                                                                        \
  std::size_t N = other.size();                                         \
  vector_register_type a,b;                                             \
  vector_register_iterator_type ia( other.data().pointer() );           \
  vector_register_iterator_type ib( this->data().pointer() );           \
  for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) { \
    ia.Next(a);                                                         \
    ib.Next(b);                                                         \
    b = b VOP a;                                                        \
    b.Stream( this->data().pointer() + i);                              \
  }                                                                     \
  return *this;                                                         \
  }                                                                     

  FETCH_ADD_OPERATOR(+=, +)
  FETCH_ADD_OPERATOR(-=, -)
  FETCH_ADD_OPERATOR(*=, *)
  FETCH_ADD_OPERATOR(/=, /)
  //  FETCH_ADD_OPERATOR(|=, |)
  //  FETCH_ADD_OPERATOR(&=, &)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(OP1, OP2)               \
  Matrix operator OP1(Matrix const &other) const { \
    Matrix ret = this->Copy();                     \
    ret OP2 other;                                 \
    return ret;                                    \
  }

  FETCH_ADD_OPERATOR(+, +=)
  FETCH_ADD_OPERATOR(-, -=)
  FETCH_ADD_OPERATOR(*, *=)
  FETCH_ADD_OPERATOR(/, /=)
  FETCH_ADD_OPERATOR(|, |=)
  FETCH_ADD_OPERATOR(&, &=)

#undef FETCH_ADD_OPERATOR

  
#define FETCH_ADD_OPERATOR(OP)                                          \
  template< typename S >                                                \
  Matrix &operator OP(S const &other) {                                 \
    for (std::size_t i = 0; i < this->data().size(); i += E_SIMD_BLOCKS) \
      for (std::size_t j = 0; j < E_SIMD_BLOCKS; ++j)                   \
        this->At(i + j) OP other;                                       \
    return *this;                                                       \
  }

  FETCH_ADD_OPERATOR(+=)
  FETCH_ADD_OPERATOR(-=)
  FETCH_ADD_OPERATOR(*=)
  FETCH_ADD_OPERATOR(/=)
  FETCH_ADD_OPERATOR(|=)
  FETCH_ADD_OPERATOR(&=)

#undef FETCH_ADD_OPERATOR

#define FETCH_ADD_OPERATOR(OP1, OP2)               \
  template< typename S >                           \
  Matrix operator OP1(S const &other) const {      \
    Matrix ret = this->Copy();                     \
    ret OP2 other;                                 \
    return ret;                                    \
  }

  FETCH_ADD_OPERATOR(+, +=)
  FETCH_ADD_OPERATOR(-, -=)
  FETCH_ADD_OPERATOR(*, *=)
  FETCH_ADD_OPERATOR(/, /=)
  FETCH_ADD_OPERATOR(|, |=)
  FETCH_ADD_OPERATOR(&, &=)

#undef FETCH_ADD_OPERATOR

  bool AllClose(Matrix const &other, double const &rtol = 1e-5, double const &atol = 1e-8) const {
    std::size_t N = this->size();
    bool ret = true;
    for (std::size_t i = 0; i < N; ++i) {
      double va = this->At(i);
      double vb = other[i];
      double vA = (va-vb);
      if(vA < 0) vA = -vA;
      if(va < 0) va = -va;
      if(vb < 0) vb = -vb;
      double M = std::max(va,vb);
      ret &= (vA < (atol + M * rtol));
    }
    return ret;
  }
  
  static Matrix Arange(type from, type const& to, double const&delta) {
    // TODO: vectorise
    assert(from < to);
    Matrix ret;    
    std::size_t N = (to - from ) / delta;
    ret.Resize(1, N);
    
    double d = from;
    for(std::size_t i = 0 ; i < N; ++i) {
      ret[i] = d;
      d += delta;
    }
    return ret;
  }

  static Matrix Zeros(std::size_t const &n) {
    Matrix ret;
    ret.Resize(n,n);
    ret.SetAllZeros();
    return ret;
  }
  
  static Matrix Zeros(std::size_t const &n, std::size_t const &m) {
    Matrix ret;
    ret.Resize(n,m);
    ret.SetAllZeros();
    return ret;
  }
  
  Matrix & InlineAdd(Matrix const &obj1) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a,b,c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    vector_register_iterator_type ib( this->data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      c = a + b;
      c.Stream( this->data().pointer() + i);
    }    

    return *this;
  }

  Matrix& InlineMultiply(Matrix const &obj1) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a,b,c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    vector_register_iterator_type ib( this->data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      c = a * b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }
  
   Matrix& InlineSubtract(Matrix const &obj1) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a,b,c; 

    vector_register_iterator_type ib( obj1.data().pointer() );
    vector_register_iterator_type ia( this->data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      c = a - b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }

   Matrix& InlineDivide(Matrix const &obj1) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a,b,c; 

    vector_register_iterator_type ib( obj1.data().pointer() );
    vector_register_iterator_type ia( this->data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      c = a / b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }
  
  
  Matrix& Transpose() {
    Matrix newm(this->width(), this->height());
    for (std::size_t i = 0; i < this->height(); ++i)
      for (std::size_t j = 0; j < this->width(); ++j)
        newm.At(j, i) = this->At(i, j);
    this->operator=(newm);
    return *this;
  }
  
  Matrix const& Add(Matrix const &obj1,   Matrix const &obj2) {
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a,b,c; 

    vector_register_iterator_type ib( obj1.data().pointer() );
    vector_register_iterator_type ia( obj2.data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      c = a + b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }

  Matrix const& Add(Matrix const &obj1, type const &scalar) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a, b(scalar), c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);

      c = a + b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }
  
  
  Matrix const& Multiply(Matrix const &obj1,   Matrix const &obj2) {
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a,b,c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    vector_register_iterator_type ib( obj2.data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      c = a * b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }

  Matrix const& Multiply(Matrix const &obj1, type const &scalar) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a, b(scalar), c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);

      c = a * b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }

  
  Matrix const& Subtract(Matrix const &obj1,   Matrix const &obj2) {
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a,b,c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    vector_register_iterator_type ib( obj2.data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      c = a - b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }

  Matrix const& Subtract(Matrix const &obj1, type const &scalar) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a, b(scalar), c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);

      c = a - b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }


  
  Matrix const& Divide(Matrix const &obj1,   Matrix const &obj2) {
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a,b,c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    vector_register_iterator_type ib( obj2.data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      c = a / b;
      c.Stream( this->data().pointer() + i);
    }    
    return *this;
  }
 
  Matrix const& Divide(Matrix const &obj1, type const &scalar) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    vector_register_type a, b(scalar), c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);

      c = a / b;
      c.Stream( this->data().pointer() + i);
    }
    
    return *this;
  }


  int Invert() {
    // after numerical recipes
    std::size_t mheight = this->height();
    std::size_t mwidth = this->width();

    type largest, inv_piv, tmp1;
    std::size_t col = 0, row = 0;

    type *ptr = this->data().pointer();

    std::vector<std::size_t> piv;
    std::vector<std::size_t> arr_col;
    std::vector<std::size_t> arr_row;
    piv.resize(mheight);
    arr_col.resize(mheight);
    arr_row.resize(mheight);

    for (std::size_t i = 0; i < mheight; ++i) piv[i] = 0;
    for (std::size_t i = 0; i < mheight; ++i) {
      largest = -1;

      // Getting pivot
      std::size_t index = 0;
      for (std::size_t j = 0; j < mheight; ++j) {
        if (piv[j] == 1)
          index += mwidth;
        else {
          for (std::size_t k = 0; k < mheight; ++k, ++index) {
            if (piv[k] == 0) {
              type d = fabs(ptr[index]);

              if (largest <= d) {
                row = j;
                col = k;
                largest = d;
              }
            }
          }
        }
      }

      std::size_t p, q;
      ++piv[col];

      if (col != row) {
        p = col * mwidth;
        q = row * mwidth;

        for (std::size_t j = 0; j < mheight; ++j, ++p, ++q)
          std::swap(ptr[p], ptr[q]);
      }

      arr_row[i] = row;
      arr_col[i] = col;

      p = col * mwidth;
      q = p + col;

      if (ptr[q] == 0)
        // TODO: Throw error
        return INVERSION_SINGULAR;

      inv_piv = 1. / ptr[q];
      ptr[q] = 1.0;
      for (std::size_t j = 0; j < mheight; ++j, ++p) ptr[p] *= inv_piv;

      p = col;
      for (std::size_t j = 0; j < mheight; p += mwidth, ++j) {
        if (j != col) {
          tmp1 = ptr[p];
          ptr[p] = 0;

          q = j * mwidth;
          std::size_t r = col * mwidth;
          for (std::size_t k = 0; k < mwidth; ++q, ++r, ++k)
            ptr[q] -= ptr[r] * tmp1;
        }
      }
    }

    // Switching cols
    for (std::size_t i = mheight - 1; i < mwidth; --i) {
      if (arr_row[i] != arr_col[i]) {
        std::size_t p = arr_col[i], q = arr_row[i];
        for (std::size_t j = 0; j < mheight; ++j, p += mwidth, q += mwidth)
          std::swap(ptr[p], ptr[q]);
      }
    }

    return INVERSION_OK;
  }

  type Sum() const {
    type ret(0);
    for (std::size_t i = 0; i < this->size(); ++i) ret += this->At(i);
    return ret;
  }

  type Max() const {
    type ret = std::numeric_limits<type>::lowest();
    for (std::size_t i = 0; i < this->size(); ++i)
      ret = std::max(ret, this->At(i));
    return ret;
  }

  type Min() const {
    type ret = std::numeric_limits<type>::max();
    for (std::size_t i = 0; i < this->size(); ++i)
      ret = std::min(ret, this->At(i));
    return ret;
  }

  type AbsMax() const {
    type ret = std::numeric_limits<type>::min();
    for (std::size_t i = 0; i < this->size(); ++i)
      ret = std::max(ret, type(fabs(this->At(i))));

    return ret;
  }

  type AbsMin() const {
    type ret = std::numeric_limits<type>::max();

    for (std::size_t i = 0; i < this->size(); ++i)
      ret = std::min(ret, type(fabs(this->At(i))));

    return ret;
  }

  type Mean() const { return Sum() / type(this->size()); }

  template< typename F >
  void Apply(F & function) {
    for(auto &a: *this)  {
      a = function(a);
    }
  }

  template< typename F >
  void SetAll(F & val) {
    for(auto &a: *this)  {
      a = val;
    }
  }
  
  
  /*
  type Variance(type const &mean) const { }
  type Variance() const { }
  void Normalise(type const &norm) {
  }
  */

  void DotReference(Matrix const &m, Matrix &ret) {
    assert(this->width() == m.height());
    ret.Resize(this->height(), m.width());

    for (std::size_t i = 0; i < ret.height(); ++i) {
      for (std::size_t j = 0; j < ret.width(); ++j) {
        type ele = 0;
        for (std::size_t k = 0; k < this->width(); ++k)
          ele += this->At(i, k) * m.At(k, j);

        ret(i, j) = ele;
      }
    }
  }

  void Dot(Matrix m, Matrix &ret) {
    m.Transpose();
    DotTransposedOf(m, ret);
    m.Transpose();
  }

  void DotTransposedOf(Matrix const &m, Matrix &ret) {
    assert(this->width() == m.width());
    ret.Resize(this->height(), m.height());

    for (std::size_t i = 0; i < ret.height(); ++i) {
      for (std::size_t j = 0; j < ret.width(); ++j) {
        type ele = 0;
        for (std::size_t k = 0; k < this->width(); ++k)
          ele += this->At(i, k) * m.At(j, k);
        ret(i, j) = ele;
      }
    }
  }

 private:
  template <std::size_t N>
  void DotImplementation(Matrix const &m, Matrix &ret) {
    // FIXME: yet to be implemented
    std::cerr << "DotImplementation in matrix not made yet!!" << std::endl;
    exit(-1);
  }
};
};
};
};
#endif
