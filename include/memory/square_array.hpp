#ifndef MEMORY_SQUARE_ARRAY_HPP
#define MEMORY_SQUARE_ARRAY_HPP
#include "shared_array.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
namespace fetch {
namespace memory {

template <typename T>
class SquareArray {
 public:
  typedef T type;
  typedef SharedArray<T> container_type;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::reverse_iterator reverse_iterator;

  SquareArray() : data_() {}
  SquareArray(std::size_t const &n) : height_(1), width_(n), data_(n) {}
  SquareArray(std::size_t const &n, std::size_t const &m)
      : height_(n), width_(m), data_(n * m) {}

  SquareArray(SquareArray &&other) = default;
  SquareArray(SquareArray const &other) = default;
  SquareArray &operator=(SquareArray const &other) = default;
  SquareArray &operator=(SquareArray &&other) = default;

  ~SquareArray() {}

  SquareArray Copy() const {
    SquareArray ret(height_, width_);
    for (std::size_t i = 0; i < size(); i += container_type::E_SIMD_COUNT) {
      for (std::size_t j = 0; j < container_type::E_SIMD_COUNT; ++j)
        ret.At(i + j) = this->At(i + j);
    }
    return ret;
  }

  void Crop(std::size_t const &i, std::size_t const &j, std::size_t const &h,
            std::size_t const &w) {
    container_type newdata(h * w);
    std::size_t m = 0;
    for (std::size_t k = i; k < i + h; ++k)
      for (std::size_t l = j; l < j + w; ++l) newdata[m++] = this->At(k, l);
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
        std::size_t v = ca * (i - ci) - sa * (j - cj) + ci;
        std::size_t u = sa * (i - ci) + ca * (j - cj) + cj;
        if ((v < height()) && (u < width()))
          n(i, j) = At(v, u);
        else
          n(i, j) = fill;
      }
    }
    data_ = n;
  }

  bool operator==(SquareArray const &other) {
    if ((height() != other.height()) || (width() != other.width()))
      return false;
    bool ret = true;

    for (std::size_t i = 0; i < data_.size(); ++i)
      ret &= (data_[i] == other.data_[i]);
    return ret;
  }

  bool operator!=(SquareArray const &other) {
    return !(this->operator==(other));
  }
  type const &operator[](std::size_t const &n) const { return data_[n]; }

  type &operator[](std::size_t const &n) { return data_[n]; }

  type const &operator()(std::size_t const &i, std::size_t const &j) const {
    assert(i < height_);
    assert(j < width_);

    return data_[(i * width_ + j)];
  }

  type &operator()(std::size_t const &i, std::size_t const &j) {
    assert(i < height_);
    assert(j < width_);
    return data_[(i * width_ + j)];
  }

  type const &At(std::size_t const &i) const { return data_[i]; }

  type const &At(std::size_t const &i, std::size_t const &j) const {
    assert(i < height_);
    assert(j < width_);

    return data_[(i * width_ + j)];
  }

  T &At(std::size_t const &i, std::size_t const &j) {
    assert(i < height_);
    assert(j < width_);
    return data_[(i * width_ + j)];
  }

  T &At(std::size_t const &i) { return data_[i]; }

  T const &Set(std::size_t const &n, T const &v) {
    assert(n < size());
    data_[n] = v;
    return v;
  }

  T const &Set(std::size_t const &i, std::size_t const &j, T const &v) {
    assert((i * width_ + j) < size());
    data_[(i * width_ + j)] = v;
    return v;
  }

  void Resize(std::size_t const &hw) { Resize(hw, hw); }

  void Resize(std::size_t const &h, std::size_t const &w) {
    if ((h == height_) && (w == width_)) return;

    container_type new_arr(h * w);
    std::size_t mH = std::min(h, height_);
    std::size_t mW = std::min(w, width_);

    for (std::size_t i = 0; i < mH; ++i) {
      for (std::size_t j = 0; j < mW; ++j) new_arr[(i * w + j)] = At(i, j);

      for (std::size_t j = mW; j < w; ++j) new_arr[(i * w + j)] = 0;
    }

    for (std::size_t i = mH; i < h; ++i)
      for (std::size_t j = 0; j < w; ++j) new_arr[(i * w + j)] = 0;

    height_ = h;
    width_ = w;
    data_ = new_arr;
  }

  void Reshape(std::size_t const &h, std::size_t const &w) {
    if (h * w != size()) {
      //      std::cerr << "TODO: throw error" <<  std::endl;
      exit(-1);
    }
    height_ = h;
    width_ = w;
  }

  void Flatten() { Reshape(1, size()); }

  iterator begin() { return data_.begin(); }
  iterator end() { return data_.end(); }
  reverse_iterator rbegin() { return data_.rbegin(); }
  reverse_iterator rend() { return data_.rend(); }

  /*
    TODO: Yet to be implemented

  template< typename T>
  SqaureArray< T > To() const {

  }


  void Save(std::byte_array const &filename) {

  }

  void Load(std::byte_array const &filename) {

  }

  */

  std::size_t height() const { return height_; }
  std::size_t width() const { return width_; }
  std::size_t size() const { return data_.size(); }

  container_type &data() { return data_; }
  container_type const &data() const { return data_; }

 private:
  std::size_t height_ = 0, width_ = 0;
  container_type data_;
};
};
};
#endif
