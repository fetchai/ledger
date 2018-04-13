#ifndef MEMORY_RECTANGULAR_ARRAY_HPP
#define MEMORY_RECTANGULAR_ARRAY_HPP
#include "memory/array.hpp"
#include "memory/shared_array.hpp"
#include "vectorize/sse.hpp"

#include "assert.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <stdio.h>

namespace fetch {
namespace memory {

template< typename T, typename C = SharedArray<T> >
class RectangularArray {
public:
  typedef T type;
  typedef C container_type;
  typedef RectangularArray< T, C > self_type;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::reverse_iterator reverse_iterator;
  typedef vectorize::VectorRegisterIterator<type,128> vector_register_iterator_type;
  
  typedef uint64_t size_type;

  typedef typename vectorize::VectorRegister< type, 128> vector_register_type;
  typedef void (*vector_kernel_type)(vector_register_type const &, vector_register_type const &, vector_register_type &);
  typedef void (*standard_kernel_type)(type const &,type const &,type &);


  
  RectangularArray() : data_() {}
  RectangularArray(std::size_t const &n) : height_(1), width_(n), data_(n) {}
  RectangularArray(std::size_t const &n, std::size_t const &m)
      : height_(n), width_(m), data_(n * m) {}

  RectangularArray(RectangularArray &&other) = default;
  RectangularArray(RectangularArray const &other) = default;
  RectangularArray &operator=(RectangularArray const &other) = default;
  RectangularArray &operator=(RectangularArray &&other) = default;

  ~RectangularArray() {}

  RectangularArray Copy() const {
    RectangularArray ret(height_, width_);

    for (size_type i = 0; i < size(); i += container_type::E_SIMD_COUNT) {
      for (size_type j = 0; j < container_type::E_SIMD_COUNT; ++j)
        ret.At(i + j) = this->At(i + j);
    }
    return ret;
  }

  void SetAllZero() {
    data_.SetAllZero();    
  }

  void SetPaddedZero() {
    data_.SetPaddedZero();    
  }
  
  void Crop(size_type const &i, size_type const &j, size_type const &h,
            size_type const &w) {
    container_type newdata(h * w);
    size_type m = 0;
    for (size_type k = i; k < i + h; ++k)
      for (size_type l = j; l < j + w; ++l) newdata[m++] = this->At(k, l);
    data_ = newdata;
    height_ = h;
    width_ = w;
  }

  void Rotate(double const &radians, type const fill = type()) {
    Rotate(radians, 0.5 * height(), 0.5 * width(), fill);
  }

  void Rotate(double const &radians, double const &ci, double const &cj,
              type const fill = type()) {
    double ca = cos(radians), sa = -sin(radians);
    container_type n(data_.size());

    for (int i = 0; i < int(height()); ++i) {
      for (int j = 0; j < int(width()); ++j) {
        size_type v = ca * (i - ci) - sa * (j - cj) + ci;
        size_type u = sa * (i - ci) + ca * (j - cj) + cj;
        if ((v < height()) && (u < width()))
          n[(i*width_+ j)] = At(v, u);
        else
          n[(i*width_+ j)] = fill;
      }
    }
    data_ = n;
  }

  bool operator==(RectangularArray const &other) {
    if ((height() != other.height()) || (width() != other.width()))
      return false;
    bool ret = true;

    for (size_type i = 0; i < data_.size(); ++i)
      ret &= (data_[i] == other.data_[i]);
    return ret;
  }

  bool operator!=(RectangularArray const &other) {
    return !(this->operator==(other));
  }
  type const &operator[](size_type const &n) const { return data_[n]; }

  type &operator[](size_type const &n) { return data_[n]; }

  type const &operator()(size_type const &i, size_type const &j) const {
    assert(i < height_);
    assert(j < width_);

    return data_[(i * width_ + j)];
  }

  type &operator()(size_type const &i, size_type const &j) {
    assert(i < height_);
    assert(j < width_);
    return data_[(i * width_ + j)];
  }

  type const &At(size_type const &i) const { return data_[i]; }

  type const &At(size_type const &i, size_type const &j) const {
    assert(i < height_);
    assert(j < width_);

    return data_[(i * width_ + j)];
  }

  T &At(size_type const &i, size_type const &j) {
    assert(i < height_);
    assert(j < width_);
    return data_[(i * width_ + j)];
  }

  T &At(size_type const &i) { return data_[i]; }

  T const &Set(size_type const &n, T const &v) {
    assert(n < size());
    data_[n] = v;
    return v;
  }

  T const &Set(size_type const &i, size_type const &j, T const &v) {
    assert((i * width_ + j) < size());
    data_[(i * width_ + j)] = v;
    return v;
  }

  void ApplyKernelElementWise(vector_kernel_type apply,
                              self_type const &obj1, self_type const &obj2) {
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();

    vector_register_type a,b,c; 

    vector_register_iterator_type ia( obj1.data().pointer() );
    vector_register_iterator_type ib( obj2.data().pointer() );    
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);
      ib.Next(b);

      apply(a,b,c);
      c.Stream( this->data().pointer() + i);

    }
  }


  void ApplyKernelElementWise(vector_kernel_type apply,
                              self_type const &obj1) {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();

    vector_register_type a,b;

    vector_register_iterator_type ia( obj1.data().pointer() );
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);

      apply(a,b);
      b.Stream( this->data().pointer() + i);
    }
  }

  void ApplyKernelElementWise(standard_kernel_type apply,
                              self_type const &obj1, self_type const &obj2)  {
    assert(obj1.data().size() == obj2.data().size());
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    
    for(std::size_t i = 0; i < N; ++i) {
      apply(obj1[i],obj2[i],data_[i]);
    }
  }

  void ApplyKernelElementWise(standard_kernel_type apply,
                              self_type const &obj1)  {
    assert(obj1.data().size() == this->data().size());

    std::size_t N = obj1.data().size();
    
    for(std::size_t i = 0; i < N; ++i) {
      apply(obj1[i],data_[i]);
    }
  }
  

 

  
  void Resize(size_type const &hw) { Resize(hw, hw); }

  void Resize(size_type const &h, size_type const &w) {
    if ((h == height_) && (w == width_)) return;

    container_type new_arr(h * w);
    size_type mH = std::min(h, height_);
    size_type mW = std::min(w, width_);

    for (size_type i = 0; i < mH; ++i) {
      for (size_type j = 0; j < mW; ++j) new_arr[(i * w + j)] = At(i, j);

      for (size_type j = mW; j < w; ++j) new_arr[(i * w + j)] = 0;
    }

    for (size_type i = mH; i < h; ++i)
      for (size_type j = 0; j < w; ++j) new_arr[(i * w + j)] = 0;

    height_ = h;
    width_ = w;
    data_ = new_arr;
  }

  void Reshape(size_type const &h, size_type const &w) {
    if (h * w != size()) {
      throw std::runtime_error("New size does not match memory - TODO, make custom exception");

    }
    height_ = h;
    width_ = w;
  }

  void Flatten() { Reshape(1, size()); }

  iterator begin() { return data_.begin(); }
  iterator end() { return data_.end(); }
  reverse_iterator rbegin() { return data_.rbegin(); }
  reverse_iterator rend() { return data_.rend(); }


  void Save(std::string const &filename) {
    FILE *fp = fopen(filename.c_str(), "wb");
    if(fp == NULL) {
      TODO_FAIL("Could not write file: ", filename);      
    }

    uint32_t magic = 0xFE7C;    
    fwrite(&magic, sizeof(magic), 1, fp); 
    fwrite(&height_, sizeof(height_), 1, fp);
    fwrite(&width_, sizeof(width_), 1, fp);
    fwrite(data_.pointer(), sizeof(type), this->size(), fp);    
    fclose(fp);    
  }

  void Load(std::string const &filename) {
    FILE *fp = fopen(filename.c_str(), "rb");
    if(fp == NULL) {
      TODO_FAIL("Could not read file: ", filename);      
    }
    
    uint32_t magic;        
    fread(&magic, sizeof(magic), 1, fp);
    if(magic != 0xFE7C) {
      TODO_FAIL("Endianess failure"); 
    }
    
    
    size_type height = 0, width = 0;
    fread(&height, sizeof(height), 1, fp);
    fread(&width, sizeof(width), 1, fp);
    
    Resize(height, width);

    fread(data_.pointer(), sizeof(type), this->size(), fp);    
    fclose(fp);    
  }

  size_type height() const { return height_; }
  size_type width() const { return width_; }
  size_type size() const { return data_.size(); }

  container_type &data() { return data_; }
  container_type const &data() const { return data_; }

 private:
  size_type height_ = 0, width_ = 0;
  container_type data_;
};
};
};
#endif
