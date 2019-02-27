#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/assert.hpp"
#include "math/shapeless_array.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include "vectorise/platform.hpp"
#include "vectorise/vectorise.hpp"

#include "math/meta/math_type_traits.hpp"

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
template <typename T, typename C = fetch::memory::SharedArray<T>, bool PAD_HEIGHT = true,
          bool PAD_WIDTH = false>
class RectangularArray : public math::ShapelessArray<T, C>
{
public:
  using super_type     = math::ShapelessArray<T, C>;
  using Type           = typename super_type::Type;
  using container_type = typename super_type::container_type;
  using SizeType       = typename super_type::SizeType;

  using self_type                     = RectangularArray<T, C>;
  using vector_register_type          = typename super_type::vector_register_type;
  using vector_register_iterator_type = typename super_type::vector_register_iterator_type;

  static constexpr char const *LOGGING_NAME = "RectangularArray";

  /* Contructs an empty rectangular array. */
  RectangularArray()
    : super_type()
  {}

  /* Contructs a rectangular array with height one.
   * @param n is the width of the array.
   *
   * The array is garantued to be aligned and a multiple of the largest
   * vector size found on the system. Space is allocated, but the
   * contructor of the underlying data structure is not invoked.
   */
  explicit RectangularArray(std::size_t const &n)
  {
    Resize(n, 1);
  }

  /* Contructs a rectangular array.
   * @param n is the height of the array.
   * @param m is the width of the array.
   *
   * The array is garantued to be aligned and a multiple of the largest
   * vector size found on the system. Space is allocated, but the
   * contructor of the underlying data structure is not invoked.
   */
  RectangularArray(std::size_t const &n, std::size_t const &m)
  {
    Resize(n, m);
  }

  RectangularArray(RectangularArray &&other)      = default;
  RectangularArray(RectangularArray const &other) = default;
  RectangularArray &operator=(RectangularArray const &other) = default;
  RectangularArray &operator=(RectangularArray &&other) = default;

  virtual ~RectangularArray() = default;

  void Sort()
  {
    std::size_t offset = 0;
    // TODO(tfr): parallelise over cores
    for (std::size_t i = 0; i < width_; ++i)
    {
      super_type::Sort(memory::TrivialRange(offset, offset + height_));
      offset += padded_height_;
    }
  }

  static RectangularArray Zeroes(std::size_t const &n, std::size_t const &m)
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
  void Crop(self_type const &A, SizeType const &i, SizeType const &h, SizeType const &j,
            SizeType const &w)
  {
    assert(this->height() == h);
    assert(this->width() == w);

    std::size_t s = 0;
    for (SizeType k = i; k < i + h; ++k)
    {
      std::size_t t = 0;
      for (SizeType l = j; l < j + w; ++l)
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
  void Rotate(double const &radians, Type const fill = Type())
  {
    Rotate(radians, 0.5 * static_cast<double>(height()), 0.5 * static_cast<double>(width()), fill);
  }

  /* Rotates the array around a point.
   * @radians is the rotation angle in radians.
   * @ci is the position along the height.
   * @cj is the position along the width.
   * @fill is the data empty entries awill be filled with.
   */
  void Rotate(double const &radians, double const &ci, double const &cj, Type const fill = Type())
  {
    assert(false);
    // TODO(tfr): FIXME, make new implementation
    double         ca = cos(radians), sa = -sin(radians);
    container_type n(super_type::data().size());

    for (int i = 0; i < int(width()); ++i)
    {
      for (int j = 0; j < int(height()); ++j)
      {
        SizeType v = SizeType(ca * (i - ci) - sa * (j - cj) + ci);
        SizeType u = SizeType(sa * (i - ci) + ca * (j - cj) + cj);
        if ((v < height()) && (u < width()))
        {
          n[std::size_t(i) * padded_height_ + std::size_t(j)] = At(v, u);
        }
        else
        {
          n[std::size_t(i) * padded_height_ + std::size_t(j)] = fill;
        }
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
    for (SizeType i = 0; i < super_type::data().size(); ++i)
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
  bool operator!=(RectangularArray const &other) const
  {
    return !(this->operator==(other));
  }

  /* One-dimensional reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that is
   * meant for non-constant object instances. Note this accessor is "slow" as
   * it takes care that the developer does not accidently enter the
   * padded area of the memory.
   */
  Type &operator[](std::size_t const &i)
  {
    return At(i);
  }

  /* One-dimensional constant reference index operator.
   * @param n is the index which is being accessed.
   *
   * This operator acts as a one-dimensional array accessor that can be
   * used for constant object instances. Note this accessor is "slow" as
   * it takes care that the developer does not accidently enter the
   * padded area of the memory.
   */
  Type const &operator[](std::size_t const &i) const
  {
    return At(i);
  }

  /* Two-dimensional constant reference index operator.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   *
   * This operator acts as a two-dimensional array accessor that can be
   * used for constant object instances.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, T>::type const &operator()(S const &i,
                                                                                 S const &j) const
  {
    assert(std::size_t(j) < padded_width_);
    assert(std::size_t(i) < padded_height_);

    return super_type::data()[(std::size_t(j) * padded_height_ + std::size_t(i))];
  }

  /* Two-dimensional reference index operator.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   *
   * This operator acts as a twoxs-dimensional array accessor that is
   * meant for non-constant object instances.
   */
  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, T>::type &operator()(S const &i, S const &j)
  {
    assert(std::size_t(j) < padded_width_);
    assert(std::size_t(i) < padded_height_);
    return super_type::data()[(std::size_t(j) * padded_height_ + std::size_t(i))];
  }

  /* One-dimensional constant reference access function.
   * @param i is the index which is being accessed.
   *
   * Note this accessor is "slow" as it takes care that the developer
   * does not accidently enter the padded area of the memory.
   */
  virtual Type const &At(SizeType const &i) const
  {
    std::size_t p = i / width_;
    std::size_t q = i % width_;

    return At(p, q);
  }

  /* One-dimensional reference access function.
   * @param i is the index which is being accessed.
   */
  virtual Type &At(SizeType const &i)
  {
    std::size_t p = i / width_;
    std::size_t q = i % width_;

    return At(p, q);
  }

  /* Two-dimensional constant reference access function.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   */
  Type const &At(SizeType const &i, SizeType const &j) const
  {
    assert(j < padded_width_);
    assert(i < padded_height_);

    return super_type::data()[(j * padded_height_ + i)];
  }

  /* Two-dimensional reference access function.
   * @param i is the index along the height direction.
   * @param j is the index along the width direction.
   */
  Type &At(SizeType const &i, SizeType const &j)
  {
    assert(j < padded_width_);
    assert(i < padded_height_);
    return super_type::data()[(j * padded_height_ + i)];
  }

  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, T> &Get(std::vector<S> const &indices)
  {
    assert(indices.size() == shape_.size());
    return this->At(indices[0], indices[1]);
  }

  template <typename S>
  fetch::meta::IfIsUnsignedInteger<S, T> const &Get(std::vector<S> const &indices) const
  {
    assert(indices.size() == shape_.size());
    return this->At(indices[0], indices[1]);
  }

  /* Sets an element using one coordinatea.
   * @param i is the position along the height.
   * @param j is the position along the width.
   * @param v is the new value.
   */
  Type const &Set(SizeType const &n, Type const &v)
  {
    return super_type::Set(n, v);
  }

  /* Sets an element using two coordinates.
   * @param i is the position along the height.
   * @param j is the position along the width.
   * @param v is the new value.
   */
  Type const &Set(SizeType const &i, SizeType const &j, Type const &v)
  {
    assert((j * padded_height_ + i) < super_type::data().size());
    super_type::data()[(j * padded_height_ + i)] = v;
    return v;
  }

  void Set(std::vector<size_t> const &indices, Type v)
  {
    assert(indices.size() == 2);
    Set(indices[0], indices[1], v);
  }

  void SetRange(std::vector<std::vector<std::size_t>> const &idxs, RectangularArray<T> const &s)
  {
    assert(idxs.size() > 0);
    assert(idxs.size() == 2);
    for (auto cur_idx : idxs)
    {
      assert(cur_idx.size() == 3);
    }

    std::size_t ret_height = (idxs[0][1] - idxs[0][0]) / idxs[0][2];
    std::size_t ret_width  = (idxs[1][1] - idxs[1][0]) / idxs[1][2];
    Reshape(ret_height, ret_width);

    std::size_t height_counter = 0;
    std::size_t width_counter;
    for (std::size_t i = idxs[0][0]; i < idxs[0][1]; i += idxs[0][2])
    {
      width_counter = 0;
      for (std::size_t j = idxs[1][0]; j < idxs[1][1]; j += idxs[1][2])
      {
        Set(height_counter, width_counter, s.At(i, j));
        ++width_counter;
      }
      ++height_counter;
    }
  }

  /* Sets an element using two coordinates.
   * @param i is the position along the height.
   * @param j is the position along the width.
   * @param v is the new value.
   *
   * This function is here to satisfy the requirement for an
   * optimisation problem container.
   */
  Type const &Insert(SizeType const &i, SizeType const &j, Type const &v)
  {
    return Set(i, j, v);
  }

  /* Resizes the array into a square array.
   * @param hw is the new height and the width of the array.
   */
  void Resize(SizeType const &hw)
  {
    Resize(hw, hw);
  }

  /* Resizes the array..
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   */
  void Resize(SizeType const &h, SizeType const &w)
  {
    if ((h == height_) && (w == width_))
    {
      return;
    }

    Reserve(h, w);

    UpdateDimensions(h, w);
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

  /* resizes based on the shape */
  void ResizeFromShape(std::vector<std::size_t> const &shape)
  {
    assert(shape.size() == 2);
    this->Resize(shape[0], shape[1]);
    this->Reshape(shape);
  }

  /* Allocates memory for the array without resizing.
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   *
   * If the new height or the width is smaller than the old, the array
   * is resized accordingly.
   */
  void Reserve(SizeType const &h, SizeType const &w)
  {

    // TODO(unknown): Rewrite
    std::size_t opw = padded_height_, ow = width_;
    std::size_t oh = height_;

    SetPaddedSizes(h, w);

    container_type new_arr(padded_width_ * padded_height_);
    new_arr.SetAllZero();

    std::size_t I = 0, J = 0;
    for (std::size_t i = 0; (i < h) && (I < oh); ++i)
    {
      for (std::size_t j = 0; j < w; ++j)
      {
        new_arr[j * padded_height_ + i] = this->data()[J * opw + I];

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

    super_type::ReplaceData(padded_width_ * padded_height_, new_arr);

    if (h < height_)
    {
      height_ = h;
    }
    if (w < width_)
    {
      width_ = w;
    }

    UpdateDimensions(height_, width_);
  }

  /**
   * reshapes the array with height and width specified separately
   * @param h array height
   * @param w array width
   */
  void Reshape(SizeType const &h, SizeType const &w)
  {
    assert((height_ * width_) == (h * w));
    Reserve(h, w);
    UpdateDimensions(h, w);
  }

  /**
   * reshapes the array with height and width specified as a vector (for compatibility with NDArray
   * methods)
   * @param shape is a vector of length 2 with height and then width.
   */
  void Reshape(std::vector<std::size_t> const &shape)
  {
    assert(shape.size() == 2);
    //    assert((shape[0] * shape[1]) == (height_ * width_));

    Reserve(shape[0], shape[1]);

    UpdateDimensions(shape[0], shape[1]);
  }

  void Flatten()
  {
    Reshape(width_ * height_, 1);
  }

  void Fill(Type const &value)
  {
    super_type::Fill(value);
  }
  void Fill(Type const & /*value*/, memory::Range const &rows, memory::Range const &cols)
  {
    std::size_t height = (rows.to() - rows.from()) / rows.step();
    std::size_t width  = (cols.to() - cols.from()) / cols.step();
    LazyResize(height, width);
    // TODO(tfr): Implement
  }

  void Fill(Type const & /*value*/, memory::TrivialRange const &rows,
            memory::TrivialRange const &cols)
  {
    std::size_t height = (rows.to() - rows.from());
    std::size_t width  = (cols.to() - cols.from());
    LazyResize(height, width);
    // TODO(tfr): Implement
  }

  /* Resizes the array into a square array in a lazy manner.
   * @param hw is the new height and the width of the array.
   *
   * This function expects that the user will take care of memory
   * initialization.
   */
  void LazyResize(SizeType const &hw)
  {
    LazyResize(hw, hw);
  }

  /* Resizes the array in a lazy manner.
   * @param h is new the height of the array.
   * @param w is new the width of the array.
   *
   * This function expects that the user will take care of memory
   * initialization.
   */
  void LazyResize(SizeType const &h, SizeType const &w)
  {
    if ((h == height_) && (w == width_))
    {
      return;
    }

    SetPaddedSizes(h, w);

    if ((padded_width_ * padded_height_) < super_type::capacity())
    {
      return;
    }

    super_type::LazyResize(padded_width_ * padded_height_);

    UpdateDimensions(h, w);

    // TODO(tfr): Take care of padded bytes
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

    if (fwrite(super_type::data().pointer(), sizeof(Type), this->padded_size(), fp) !=
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

    SizeType              height = 0, width = 0;
    std::vector<SizeType> shape{0, 0};
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

    if (fread(super_type::data().pointer(), sizeof(Type), this->padded_size(), fp) !=
        (this->padded_size()))
    {
      TODO_FAIL("failed to read body of matrix - TODO, ,make custom exception");
    }

    fclose(fp);
  }

  /* Returns the height of the array. */
  SizeType height() const
  {
    return height_;
  }

  /* Returns the width of the array. */
  SizeType width() const
  {
    return width_;
  }

  /* Returns height, width of array */
  std::vector<SizeType> shape() const
  {
    return shape_;
  }

  /* Returns the padded height of the array. */
  SizeType padded_height() const
  {
    return padded_height_;
  }

  /* Returns the padded width of the array. */
  SizeType padded_width() const
  {
    return padded_width_;
  }

  /* Returns the size of the array. */
  SizeType size() const
  {
    assert(this->size_ == (height_ * width_));
    return height_ * width_;
  }

  /* Returns the size of the array. */
  SizeType padded_size() const
  {
    return padded_width_ * padded_height_;
  }

  /**
   * overrides the ShapelessArray AllClose because of padding
   * @param other array to compare to
   * @param rtol relative tolerance
   * @param atol absolute tolerance
   * @param ignoreNaN flag for ignoring NaNs
   * @return true if all values in both arrays are close enough
   */
  bool AllClose(self_type const &other, Type const &rtol = Type(1e-5),
                Type const &atol = Type(1e-8), bool ignoreNaN = true) const
  {
    std::size_t N = this->size();
    if (other.size() != N)
    {
      return false;
    }
    bool ret = true;
    for (std::size_t i = 0; ret && (i < N); ++i)
    {
      Type va = this->At(i);
      if (ignoreNaN && std::isnan(va))
      {
        continue;
      }
      Type vb = other[i];
      if (ignoreNaN && std::isnan(vb))
      {
        continue;
      }
      Type vA = (va - vb);
      if (vA < 0)
      {
        vA = -vA;
      }
      if (va < 0)
      {
        va = -va;
      }
      if (vb < 0)
      {
        vb = -vb;
      }
      Type M = std::max(va, vb);

      ret = (vA <= std::max(atol, M * rtol));
    }
    if (!ret)
    {
      for (std::size_t i = 0; i < N; ++i)
      {
        Type va = this->At(i);
        if (ignoreNaN && std::isnan(va))
        {
          continue;
        }
        Type vb = other[i];
        if (ignoreNaN && std::isnan(vb))
        {
          continue;
        }
        Type vA = (va - vb);
        if (vA < 0)
        {
          vA = -vA;
        }
        if (va < 0)
        {
          va = -va;
        }
        if (vb < 0)
        {
          vb = -vb;
        }
        Type M = std::max(va, vb);
        std::cout << this->At(i) << " " << other[i] << " "
                  << ((vA < std::max(atol, M * rtol)) ? " " : "*") << std::endl;
      }
    }

    return ret;
  }

private:
  SizeType              height_ = 0, width_ = 0;
  std::vector<SizeType> shape_{0, 0};
  SizeType              padded_width_ = 0, padded_height_ = 0;

  void SetPaddedSizes(SizeType const &h, SizeType const &w)
  {
    padded_width_  = w;
    padded_height_ = h;

    if (PAD_WIDTH)
    {
      padded_width_ =
          SizeType(w / vector_register_type::E_BLOCK_COUNT) * vector_register_type::E_BLOCK_COUNT;
      if (padded_width_ < w)
      {
        padded_width_ += vector_register_type::E_BLOCK_COUNT;
      }
    }

    if (PAD_HEIGHT)
    {
      padded_height_ =
          SizeType(h / vector_register_type::E_BLOCK_COUNT) * vector_register_type::E_BLOCK_COUNT;
      if (padded_height_ < h)
      {
        padded_height_ += vector_register_type::E_BLOCK_COUNT;
      }
    }
  }

  /**
   * helper method for setting all shape and size values correctly internally
   * @param height
   * @param width
   */
  void UpdateDimensions(std::size_t height, std::size_t width)
  {
    height_     = height;
    width_      = width;
    shape_      = {height_, width_};
    this->size_ = height_ * width_;
  }
};
}  // namespace math
}  // namespace fetch
