#ifndef MEMORY_RECTANGULAR_ARRAY_HPP
#define MEMORY_RECTANGULAR_ARRAY_HPP
#include "memory/array.hpp"
#include "memory/shared_array.hpp"
#include "vectorize/sse.hpp"
#include "platform.hpp"

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
  typedef uint64_t size_type;
  typedef RectangularArray< T, C > self_type;


  /* Iterators for accessing and modifying the array */
  typedef typename container_type::iterator iterator;
  typedef typename container_type::reverse_iterator reverse_iterator;

  /* Vector register is used parallel instruction execution using SIMD */
  typedef typename vectorize::VectorRegister< type, platform::vector_size > vector_register_type;
  typedef vectorize::VectorRegisterIterator<type, platform::vector_size> vector_register_iterator_type;
  
  /* Kernels for performing execution */
  typedef void (*vector_kernel_type)(vector_register_type const &, vector_register_type const &, vector_register_type &);
  typedef void (*standard_kernel_type)(type const &,type const &,type &);

  /* Contructs an empty rectangular array. */  
  RectangularArray() : data_() {}

  /* Contructs a rectangular array with height one.
   * @param n is the width of the array.
   *
   * The array is garantued to be aligned and a multiple of the largest
   * vector size found on the system. Space is allocated, but the
   * contructor of the underlying data structure is not invoked.
   */
  RectangularArray(std::size_t const &n) : height_(1), width_(n), data_(n) {}

  /* Contructs a rectangular array.
   * @param n is the height of the array.
   * @param m is the width of the array.
   *
   * The array is garantued to be aligned and a multiple of the largest
   * vector size found on the system. Space is allocated, but the
   * contructor of the underlying data structure is not invoked.
   */  
  RectangularArray(std::size_t const &n, std::size_t const &m)
      : height_(n), width_(m), data_(n * m) {}

  RectangularArray(RectangularArray &&other) = default;
  RectangularArray(RectangularArray const &other) = default;
  RectangularArray &operator=(RectangularArray const &other) = default;
  RectangularArray &operator=(RectangularArray &&other) = default;

  ~RectangularArray() {}

  /* Makes a copy of the array.
   *
   * The copy is shallow in the sense that it does not make copies of
   * underying data structures even if the contain a copy function.
   */    
  RectangularArray Copy() const {
    RectangularArray ret(height_, width_);

    for (size_type i = 0; i < size(); i += container_type::E_SIMD_COUNT) {
      for (size_type j = 0; j < container_type::E_SIMD_COUNT; ++j)
        ret.At(i + j) = this->At(i + j);
    }
    return ret;
  }

  /* Set all elements to zero.
   *
   * This method will initialise all memory with zero.
   */
  void SetAllZero() {
    data_.SetAllZero();    
  }

  /* Set all padded bytes to zero.
   *
   * This method sets the padded bytes to zero. Padded bytes are those
   * which are added to ensure that the arrays true size is a multiple
   * of the vector unit.
   */  
  void SetPaddedZero() {
    data_.SetPaddedZero();    
  }


  /* Crops the current array.
   * @param i is the starting coordinate along the height direction.
   * @param j is the starting coordinate along the width direction.
   * @param h is the crop height.
   * @param w is the crop width.
   *
   * This method allocates new array to make the crop. (TODO: Reuse
   * memory for effiency, if array and not sharedarray is used)
   */
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

  /* Rotates the array around the center.
   * @radians is the rotation angle in radians.
   * @fill is the data empty entries awill be filled with.
   */
  void Rotate(double const &radians, type const fill = type()) {
    Rotate(radians, 0.5 * height(), 0.5 * width(), fill);
  }


  /* Rotates the array around a point.
   * @radians is the rotation angle in radians.
   * @ci is the position along the height.
   * @cj is the position along the width.
   * @fill is the data empty entries awill be filled with.
   */  
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

  /* Equality operator.
   * @other is the array which this instance is compared against.
   *
   * This method is sensitive to height and width. 
   */
  bool operator==(RectangularArray const &other) {
    if ((height() != other.height()) || (width() != other.width()))
      return false;
    bool ret = true;

    for (size_type i = 0; i < data_.size(); ++i)
      ret &= (data_[i] == other.data_[i]);
    return ret;
  }

  /* Not-equal operator.
   * @other is the array which this instance is compared against.
   *
   * This method is sensitive to height and width. 
   */  
  bool operator!=(RectangularArray const &other) {
    return !(this->operator==(other));
  }

  /* One-dimensional constant reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that can be
   * used for constant object instances.
   */
  type const &operator[](size_type const &n) const { return data_[n]; }

  /* One-dimensional reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that is
   * meant for non-constant object instances.
   */  
  type &operator[](size_type const &n) { return data_[n]; }

  /* Two-dimensional constant reference index operator.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   *
   * This operator acts as a two-dimensional array accessor that can be
   * used for constant object instances.
   */  
  type const &operator()(size_type const &i, size_type const &j) const {
    assert(i < height_);
    assert(j < width_);

    return data_[(i * width_ + j)];
  }

  /* Two-dimensional reference index operator.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   *
   * This operator acts as a twoxs-dimensional array accessor that is
   * meant for non-constant object instances.
   */    
  type &operator()(size_type const &i, size_type const &j) {
    assert(i < height_);
    assert(j < width_);
    return data_[(i * width_ + j)];
  }

  /* One-dimensional constant reference access function.
   * @param i is the index which is being accessed.
   */
  type const &At(size_type const &i) const { return data_[i]; }

  /* Two-dimensional constant reference access function.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   */    
  type const &At(size_type const &i, size_type const &j) const {
    assert(i < height_);
    assert(j < width_);

    return data_[(i * width_ + j)];
  }

  /* Two-dimensional reference access function.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   */    
  type &At(size_type const &i, size_type const &j) {
    assert(i < height_);
    assert(j < width_);
    return data_[(i * width_ + j)];
  }

  /* One-dimensional reference access function.
   * @param i is the index which is being accessed.
   */  
  type &At(size_type const &i) { return data_[i]; }

  /* Sets an element using one coordinatea.
   * @param i is the position along the height.
   * @param j is the position along the width.
   * @param v is the new value.
   */  
  type const &Set(size_type const &n, type const &v) {
    assert(n < size());
    data_[n] = v;
    return v;
  }

  /* Sets an element using two coordinates.
   * @param i is the position along the height.
   * @param j is the position along the width.
   * @param v is the new value.
   */
  type const &Set(size_type const &i, size_type const &j, type const &v) {
    assert((i * width_ + j) < size());
    data_[(i * width_ + j)] = v;
    return v;
  }

  /* Applies a vector kernel to each array element.
   * @param apply is the kernel which will be applied
   * @param obj1 is an array containing the first kernel arguments.
   * @param obj2 is an array containing the second kernel arguments.
   *
   * This method updates each element of the array using two other
   * arrays elements at the same positions. The method is vectorised and
   * uses the platforms vector registers if available.
   */
  void ApplyKernelElementWise(vector_kernel_type apply,
                              self_type const &obj1, self_type const &obj2) {
    assert(obj1.size() == obj2.size());
    assert(obj1.size() == this->size());

    std::size_t N = obj1.size();

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


  /* Applies a vector kernel to each array element.
   * @param apply is the kernel which will be applied
   * @param obj1 is an array containing the first kernel arguments.
   *
   * This method updates each element of the array using the elements of
   * one other at the same positions. The method is vectorised and
   * uses the platforms vector registers if available.
   */  
  void ApplyKernelElementWise(vector_kernel_type apply,
                              self_type const &obj1) {
    assert(obj1.size() == this->size());

    std::size_t N = obj1.size();

    vector_register_type a,b;

    vector_register_iterator_type ia( obj1.data().pointer() );
    for(std::size_t i = 0; i < N; i += vector_register_type::E_BLOCK_COUNT) {
      ia.Next(a);

      apply(a,b);
      b.Stream( this->data().pointer() + i);
    }
  }

  /* Applies a standard kernel to each array element.
   * @param apply is the kernel which will be applied
   * @param obj1 is an array containing the first kernel arguments.
   * @param obj2 is an array containing the first kernel arguments.
   *
   * This method updates each element of the array using the elements of
   * two other arrays at the same positions. The method is not vectorised.
   */    
  void ApplyKernelElementWise(standard_kernel_type apply,
                              self_type const &obj1, self_type const &obj2)  {
    assert(obj1.size() == obj2.size());
    assert(obj1.size() == this->size());

    std::size_t N = obj1.size();
    
    for(std::size_t i = 0; i < N; ++i) {
      apply(obj1[i],obj2[i],data_[i]);
    }
  }

  /* Applies a standard kernel to each array element.
   * @param apply is the kernel which will be applied
   * @param obj1 is an array containing the first kernel arguments.
   *
   * This method updates each element of the array using the elements of
   * one other array at the same positions. The method is not vectorised.
   */      
  void ApplyKernelElementWise(standard_kernel_type apply,
                              self_type const &obj1)  {
    assert(obj1.size() == this->size());

    std::size_t N = obj1.size();
    
    for(std::size_t i = 0; i < N; ++i) {
      apply(obj1[i],data_[i]);
    }
  }
  

  /* Resizes the array into a square array.
   * @param hw is the new height and the width of the array.
   */
  void Resize(size_type const &hw) { Resize(hw, hw); }

  /* Resizes the array..
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   */  
  void Resize(size_type const &h, size_type const &w) {
    if ((h == height_) && (w == width_)) return;

    Reserve(h, w);
    
    height_ = h;
    width_ = w;
  }
  

  /* Allocates memory for the array without resizing.
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   *
   * If the new height or the width is smaller than the old, the array
   * is resized accordingly.
   */    
  void Reserve(size_type const &h, size_type const &w) {
    container_type new_arr(h * w);
    size_type mH = std::min(h, height_);
    size_type mW = std::min(w, width_);

    for (size_type i = 0; i < mH; ++i) {
      for (size_type j = 0; j < mW; ++j) new_arr[(i * w + j)] = At(i, j);

      for (size_type j = mW; j < w; ++j) new_arr[(i * w + j)] = 0;
    }

    for (size_type i = mH; i < h; ++i)
      for (size_type j = 0; j < w; ++j) new_arr[(i * w + j)] = 0;
    data_ = new_arr;
    
    if(h < height_) height_ = h;  
    if(w < width_) width_ = w;  
  }


  /* Resizes the array into a square array in a lazy manner.
   * @param hw is the new height and the width of the array.
   *
   * This function expects that the user will take care of memory
   * initialization. 
   */
  void LazyResize(size_type const &hw) { Resize(hw, hw); }

  /* Resizes the array in a lazy manner.
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   *
   * This function expects that the user will take care of memory
   * initialization. 
   */  
  void LazyResize(size_type const &h, size_type const &w) {
    if ((h == height_) && (w == width_)) return;

    LazyReserve(h, w);
    
    height_ = h;
    width_ = w;

    // TODO: Take care of padded bytes
  }

  /* Allocates memory for the array in a lazy manner.
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   *
   * This function expects that the user will take care of memory
   * initialization. 
   */    
  void LazyReserve(size_type const &h, size_type const &w) {
    if( (h * w) < capacity()) return;

    container_type new_arr(h * w);
    data_ = new_arr; 
  }

  /* Reshapes the array to a new height and width.
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   *
   * Padded bytes are set to zero.
   */      
  void Reshape(size_type const &h, size_type const &w) {
    if (h * w >= size()) {
      throw std::runtime_error("New size does not match memory - TODO, make custom exception");
    }
    height_ = h;
    width_ = w;

    // TODO: Set padded bytes to zero
  }

  void Flatten() { Reshape(1, size()); }

  iterator begin() { return data_.begin(); }
  iterator end() { return data_.end(); }
  reverse_iterator rbegin() { return data_.rbegin(); }
  reverse_iterator rend() { return data_.rend(); }


  /* Saves the array into a file.
   * @param filename is the filename.  
   */        
  void Save(std::string const &filename) {
    FILE *fp = fopen(filename.c_str(), "wb");
    if(fp == NULL) {
      TODO_FAIL("Could not write file: ", filename);      
    }

    uint16_t magic = 0xFE7C;    
    fwrite(&magic, sizeof(magic), 1, fp); 
    fwrite(&height_, sizeof(height_), 1, fp);
    fwrite(&width_, sizeof(width_), 1, fp);
    fwrite(data_.pointer(), sizeof(type), this->size(), fp);    
    fclose(fp);    
  }

  /* Load the array from a file.
   * @param filename is the filename.  
   *
   * Currently, this code does not correct for wrong endianess (TODO).
   */  
  void Load(std::string const &filename) {
    FILE *fp = fopen(filename.c_str(), "rb");
    if(fp == NULL) {
      TODO_FAIL("Could not read file: ", filename);      
    }
    
    uint16_t magic;        
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
  
  /* Returns the height of the array. */
  size_type height() const { return height_; }

  /* Returns the width of the array. */  
  size_type width() const { return width_; }

  /* Returns the size of the array. */    
  size_type size() const { return height_ * width_; }

  /* Returns the capacity of the array. */    
  size_type capacity() const { return data_.padded_size(); }
  
  /* Returns a reference to the underlying array. */      
  container_type &data() { return data_; }

  /* Returns a constant reference to the underlying array. */        
  container_type const &data() const { return data_; }

 private:
  size_type height_ = 0, width_ = 0;
  container_type data_;
};
};
};
#endif
