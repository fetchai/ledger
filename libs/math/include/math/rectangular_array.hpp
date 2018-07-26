#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include "vectorise/platform.hpp"
#include "vectorise/vectorise.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <stdio.h>

namespace fetch {
namespace math {

/* A class that contains an array that is suitable for vectorisation.
 *
 * The RectangularArray class offers optional height and width padding
 * to ensure that the corresponding array is suitable for
 * vectorization. The allocated memory is garantueed to be aligned
 * according to the platform standard by using either
 * <fetch::memory::SharedArray> or <fetch::memory::Array>.
 */
template <typename T, typename C = memory::SharedArray<T>, bool PAD_HEIGHT = false,
          bool PAD_WIDTH = true>
class RectangularArray : public math::ShapeLessArray<T, C>
{
public:
  typedef math::ShapeLessArray<T, C>          super_type;
  typedef typename super_type::type           type;
  typedef typename super_type::container_type container_type;
  typedef typename super_type::size_type      size_type;

  typedef RectangularArray<T, C>                             self_type;
  typedef typename super_type::vector_register_type          vector_register_type;
  typedef typename super_type::vector_register_iterator_type vector_register_iterator_type;

  /* Contructs an empty rectangular array. */
  RectangularArray() : super_type() {}

  /* Contructs a rectangular array with height one.
   * @param n is the width of the array.
   *
   * The array is garantued to be aligned and a multiple of the largest
   * vector size found on the system. Space is allocated, but the
   * contructor of the underlying data structure is not invoked.
   */
  RectangularArray(std::size_t const &n) { Resize(1, n); }

  /* Contructs a rectangular array.
   * @param n is the height of the array.
   * @param m is the width of the array.
   *
   * The array is garantued to be aligned and a multiple of the largest
   * vector size found on the system. Space is allocated, but the
   * contructor of the underlying data structure is not invoked.
   */
  RectangularArray(std::size_t const &n, std::size_t const &m) { Resize(n, m); }

  RectangularArray(RectangularArray &&other)      = default;
  RectangularArray(RectangularArray const &other) = default;
  RectangularArray &operator=(RectangularArray const &other) = default;
  RectangularArray &operator=(RectangularArray &&other) = default;

  ~RectangularArray() {}

  void Sort()
  {
    std::size_t offset = 0;
    // TODO: parallelise over cores
    for (std::size_t i = 0; i < height_; ++i)
    {
      super_type::Sort(memory::TrivialRange(offset, offset + width_));
      offset += padded_width_;
    }
  }

  static RectangularArray Zeros(std::size_t const &n, std::size_t const &m)
  {
    RectangularArray ret;
    ret.LazyResize(n, m);
    ret.data().SetAllZero();
    return ret;
  }

  static RectangularArray UniformRandom(std::size_t const &n, std::size_t const &m)
  {
    RectangularArray ret;

    ret.LazyResize(n, m);
    ret.FillUniformRandom();

    ret.SetPaddedZero();

    return ret;
  }

  /* Crops the current array.
   * @param A is the original array.
   * @param i is the starting coordinate along the height direction.
   * @param h is the crop height.
   * @param j is the starting coordinate along the width direction.
   * @param w is the crop width.
   *
   * This method allocates new array to make the crop. (TODO: Reuse
   * memory for effiency, if array and not sharedarray is used)
   */
  void Crop(self_type const &A, size_type const &i, size_type const &h, size_type const &j,
            size_type const &w)
  {
    assert(this->height() == h);
    assert(this->width() == w);

    std::size_t s = 0;
    for (size_type k = i; k < i + h; ++k)
    {
      std::size_t t = 0;
      for (size_type l = j; l < j + w; ++l)
      {
        this->At(s, t) = A.At(k, l);
        ++t;
      }
      ++s;
    }
  }

  void Column(RectangularArray const &obj1, std::size_t const &i)
  {
    this->Crop(obj1, 0, height(), i, 1);
  }

  void Column(RectangularArray const &obj1, memory::TrivialRange const &range)
  {
    assert(range.step() == 1);
    assert(range.to() < width());

    this->Crop(obj1, 0, height(), range.from(), range.to() - range.from());
  }

  void Row(RectangularArray const &obj1, std::size_t const &i)
  {
    this->Crop(obj1, i, 1, 0, width());
  }

  void Row(RectangularArray const &obj1, memory::TrivialRange const &range)
  {
    assert(range.step() == 1);
    assert(range.to() < height());

    this->Crop(obj1, range.from(), range.to() - range.from(), 0, width());
  }

  template <typename G>
  //  typename std::enable_if< std::is_same< type, typename G::type >::value,
  //  void >::type
  void Copy(G const &orig)
  {
    assert(orig.height() == height_);
    assert(orig.width() == width_);

    for (std::size_t i = 0; i < orig.height(); ++i)
    {
      for (std::size_t j = 0; j < orig.width(); ++j)
      {
        this->At(i, j) = orig.At(i, j);
      }
    }
  }

  /* Rotates the array around the center.
   * @radians is the rotation angle in radians.
   * @fill is the data empty entries awill be filled with.
   */
  void Rotate(double const &radians, type const fill = type())
  {
    Rotate(radians, 0.5 * height(), 0.5 * width(), fill);
  }

  /* Rotates the array around a point.
   * @radians is the rotation angle in radians.
   * @ci is the position along the height.
   * @cj is the position along the width.
   * @fill is the data empty entries awill be filled with.
   */
  void Rotate(double const &radians, double const &ci, double const &cj, type const fill = type())
  {
    assert(false);
    // TODO: FIXME, make new implementation
    double         ca = cos(radians), sa = -sin(radians);
    container_type n(super_type::data().size());

    for (int i = 0; i < int(height()); ++i)
    {
      for (int j = 0; j < int(width()); ++j)
      {
        size_type v = size_type(ca * (i - ci) - sa * (j - cj) + ci);
        size_type u = size_type(sa * (i - ci) + ca * (j - cj) + cj);
        if ((v < height()) && (u < width()))
          n[std::size_t(i) * padded_width_ + std::size_t(j)] = At(v, u);
        else
          n[std::size_t(i) * padded_width_ + std::size_t(j)] = fill;
      }
    }
    //    data_ = n;
  }

  /* Equality operator.
   * @other is the array which this instance is compared against.
   *
   * This method is sensitive to height and width.
   */
  bool operator==(RectangularArray const &other) const
  {
    if ((height() != other.height()) || (width() != other.width()))
    {
      return false;
    }
    bool ret = true;

    // FIXME: Implementation wrong due to padding
    for (size_type i = 0; i < super_type::data().size(); ++i)
    {
      ret &= (super_type::data()[i] == other.data()[i]);
    }
    return ret;
  }

  /* Not-equal operator.
   * @other is the array which this instance is compared against.
   *
   * This method is sensitive to height and width.
   */
  bool operator!=(RectangularArray const &other) const { return !(this->operator==(other)); }

  /* One-dimensional reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that is
   * meant for non-constant object instances. Note this accessor is "slow" as
   * it takes care that the developer does not accidently enter the
   * padded area of the memory.
   */
  type &operator[](std::size_t const &i) { return At(i); }

  /* One-dimensional constant reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that can be
   * used for constant object instances. Note this accessor is "slow" as
   * it takes care that the developer does not accidently enter the
   * padded area of the memory.
   */
  type const &operator[](std::size_t const &i) const { return At(i); }

  /* Two-dimensional constant reference index operator.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   *
   * This operator acts as a two-dimensional array accessor that can be
   * used for constant object instances.
   */
  type const &operator()(size_type const &i, size_type const &j) const
  {
    assert(i < padded_height_);
    assert(j < padded_width_);

    return super_type::data()[(i * padded_width_ + j)];
  }

  /* Two-dimensional reference index operator.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   *
   * This operator acts as a twoxs-dimensional array accessor that is
   * meant for non-constant object instances.
   */
  type &operator()(size_type const &i, size_type const &j)
  {
    assert(i < padded_height_);
    assert(j < padded_width_);
    return super_type::data()[(i * padded_width_ + j)];
  }

  /* One-dimensional constant reference access function.
   * @param i is the index which is being accessed.
   *
   * Note this accessor is "slow" as it takes care that the developer
   * does not accidently enter the padded area of the memory.
   */
  type const &At(size_type const &i) const
  {
    std::size_t p = i / width_;
    std::size_t q = i % width_;

    return At(p, q);
  }

  /* One-dimensional reference access function.
   * @param i is the index which is being accessed.
   */
  type &At(size_type const &i)
  {
    std::size_t p = i / width_;
    std::size_t q = i % width_;

    return At(p, q);
  }

  /* Two-dimensional constant reference access function.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   */
  type const &At(size_type const &i, size_type const &j) const
  {
    assert(i < padded_height_);
    assert(j < padded_width_);

    return super_type::data()[(i * padded_width_ + j)];
  }

  /* Two-dimensional reference access function.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   */
  type &At(size_type const &i, size_type const &j)
  {
    assert(i < padded_height_);
    assert(j < padded_width_);
    return super_type::data()[(i * padded_width_ + j)];
  }

  /* Sets an element using one coordinatea.
   * @param i is the position along the height.
   * @param j is the position along the width.
   * @param v is the new value.
   */
  type const &Set(size_type const &n, type const &v)
  {
    assert(n < super_type::data().size());
    super_type::data()[n] = v;
    return v;
  }

  /* Sets an element using two coordinates.
   * @param i is the position along the height.
   * @param j is the position along the width.
   * @param v is the new value.
   */
  type const &Set(size_type const &i, size_type const &j, type const &v)
  {
    assert((i * padded_width_ + j) < super_type::data().size());
    super_type::data()[(i * padded_width_ + j)] = v;
    return v;
  }

  /* Sets an element using two coordinates.
   * @param i is the position along the height.
   * @param j is the position along the width.
   * @param v is the new value.
   *
   * This function is here to satisfy the requirement for an
   * optimisation problem container.
   */
  type const &Insert(size_type const &i, size_type const &j, type const &v) { return Set(i, j, v); }

  /* Resizes the array into a square array.
   * @param hw is the new height and the width of the array.
   */
  void Resize(size_type const &hw) { Resize(hw, hw); }

  /* Resizes the array..
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   */
  void Resize(size_type const &h, size_type const &w)
  {
    if ((h == height_) && (w == width_)) return;

    Reserve(h, w);

    height_ = h;
    width_  = w;
  }

  void Resize(std::vector<std::size_t> const &shape, std::size_t const &offset = 0)
  {

    switch ((shape.size() - offset))
    {
    case 2:
      Resize(shape[offset], shape[offset + 1]);
      break;
    case 1:
      Resize(1, shape[offset]);
      break;
    default:
      assert(false);
      break;
    }
  }

  /* Allocates memory for the array without resizing.
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   *
   * If the new height or the width is smaller than the old, the array
   * is resized accordingly.
   */
  void Reserve(size_type const &h, size_type const &w)
  {
    std::size_t opw = padded_width_, ow = width_;
    std::size_t oh = height_;

    SetPaddedSizes(h, w);

    container_type new_arr(padded_height_ * padded_width_);
    new_arr.SetAllZero();

    std::size_t I = 0, J = 0;
    for (std::size_t i = 0; (i < h) && (I < oh); ++i)
    {
      for (std::size_t j = 0; j < w; ++j)
      {
        new_arr[i * padded_width_ + j] = this->data()[I * opw + J];

        ++J;
        if (J == ow)
        {
          ++I;
          J = 0;
          if (I == oh)
          {
            break;
          }
        }
      }
    }

    super_type::ReplaceData(padded_height_ * padded_width_, new_arr);

    if (h < height_) height_ = h;
    if (w < width_) width_ = w;
  }

  void Reshape(size_type const &h, size_type const &w)
  {
    assert((height_ * width_) == (h * w));
    Reserve(h, w);

    height_ = h;
    width_  = w;
  }

  void Flatten() { Reshape(1, width_ * height_); }

  void Fill(type const &value, memory::Range const &rows, memory::Range const &cols)
  {
    std::size_t height = (rows.to() - rows.from()) / rows.step();
    std::size_t width  = (cols.to() - cols.from()) / cols.step();
    LazyResize(height, width);
    // TODO: Implement
  }

  void Fill(type const &value, memory::TrivialRange const &rows, memory::TrivialRange const &cols)
  {
    std::size_t height = (rows.to() - rows.from());
    std::size_t width  = (cols.to() - cols.from());
    LazyResize(height, width);
    // TODO: Implement
  }

  /* Resizes the array into a square array in a lazy manner.
   * @param hw is the new height and the width of the array.
   *
   * This function expects that the user will take care of memory
   * initialization.
   */
  void LazyResize(size_type const &hw) { LazyResize(hw, hw); }

  /* Resizes the array in a lazy manner.
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   *
   * This function expects that the user will take care of memory
   * initialization.
   */
  void LazyResize(size_type const &h, size_type const &w)
  {
    if ((h == height_) && (w == width_)) return;

    SetPaddedSizes(h, w);

    if ((padded_height_ * padded_width_) < super_type::capacity()) return;

    super_type::LazyResize(padded_height_ * padded_width_);

    height_ = h;
    width_  = w;

    // TODO: Take care of padded bytes
  }

  /* Saves the array into a file.
   * @param filename is the filename.
   */
  void Save(std::string const &filename)
  {
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == NULL)
    {
      TODO_FAIL("Could not write file: ", filename);
    }

    uint16_t magic = 0xFE7C;
    if (fwrite(&magic, sizeof(magic), 1, fp) != 1)
    {
      TODO_FAIL("Could not write magic - todo: make custom exception");
    }

    if (fwrite(&height_, sizeof(height_), 1, fp) != 1)
    {
      TODO_FAIL("Could not write height - todo: make custom exception");
    }

    if (fwrite(&width_, sizeof(width_), 1, fp) != 1)
    {
      TODO_FAIL("Could not write width - todo: make custom exc");
    }

    if (fwrite(super_type::data().pointer(), sizeof(type), this->padded_size(), fp) !=
        this->padded_size())
    {
      TODO_FAIL("Could not write matrix body - todo: make custom exc");
    }
    fclose(fp);
  }

  /* Load the array from a file.
   * @param filename is the filename.
   *
   * Currently, this code does not correct for wrong endianess (TODO).
   */
  void Load(std::string const &filename)
  {
    FILE *fp = fopen(filename.c_str(), "rb");
    if (fp == NULL)
    {
      TODO_FAIL("Could not read file: ", filename);
    }

    uint16_t magic;
    if (fread(&magic, sizeof(magic), 1, fp) != 1)
    {
      TODO_FAIL("Could not read magic - throw custom exception");
    }

    if (magic != 0xFE7C)
    {
      TODO_FAIL("Endianess failure");
    }

    size_type height = 0, width = 0;
    if (fread(&height, sizeof(height), 1, fp) != 1)
    {
      TODO_FAIL(
          "failed to read height of matrix - TODO, make custom exception for "
          "this");
    }

    if (fread(&width, sizeof(width), 1, fp) != 1)
    {
      TODO_FAIL(
          "failed to read width of matrix - TODO, make custom exception for "
          "this");
    }

    Resize(height, width);

    if (fread(super_type::data().pointer(), sizeof(type), this->padded_size(), fp) !=
        (this->padded_size()))
    {
      TODO_FAIL("failed to read body of matrix - TODO, ,make custom exception");
    }

    fclose(fp);
  }

  /* Returns the height of the array. */
  size_type height() const { return height_; }

  /* Returns the width of the array. */
  size_type width() const { return width_; }

  /* Returns the padded height of the array. */
  size_type padded_height() const { return padded_height_; }

  /* Returns the padded width of the array. */
  size_type padded_width() const { return padded_width_; }

  /* Returns the size of the array. */
  size_type size() const { return height_ * width_; }

  /* Returns the size of the array. */
  size_type padded_size() const { return padded_height_ * padded_width_; }

private:
  size_type height_ = 0, width_ = 0;
  size_type padded_height_ = 0, padded_width_ = 0;

  void SetPaddedSizes(size_type const &h, size_type const &w)
  {
    padded_height_ = h;
    padded_width_  = w;

    if (PAD_HEIGHT)
    {
      padded_height_ =
          size_type(h / vector_register_type::E_BLOCK_COUNT) * vector_register_type::E_BLOCK_COUNT;
      if (padded_height_ < h) padded_height_ += vector_register_type::E_BLOCK_COUNT;
    }

    if (PAD_WIDTH)
    {
      padded_width_ =
          size_type(w / vector_register_type::E_BLOCK_COUNT) * vector_register_type::E_BLOCK_COUNT;
      if (padded_width_ < w) padded_width_ += vector_register_type::E_BLOCK_COUNT;
    }
  }
};
}  // namespace math
}  // namespace fetch
