//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "math/base_types.hpp"
#include "math/matrix_operations.hpp"
#include "math/standard_functions/abs.hpp"
#include "math/tensor/tensor.hpp"
#include "math/tensor/tensor_broadcast.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/memory/array.hpp"

namespace fetch {
namespace math {

namespace {
template <typename DataType, typename ArrayType>
static void ArangeImplementation(DataType const &from, DataType const &to, DataType const &delta,
                                 ArrayType &ret)
{
  auto N = SizeType((to - from) / delta);
  ret.Resize({N});
  ret.FillArange(static_cast<typename ArrayType::Type>(from),
                 static_cast<typename ArrayType::Type>(to));
}
}  // namespace

///////////////////////////
/// TENSOR CONSTRUCTORS ///
///////////////////////////

/**
 * Constructor builds an Tensor with n elements initialised to 0
 * @param n   number of elements in array (no shape specified, assume 1-D)
 */
template <typename T, typename C>
Tensor<T, C>::Tensor(SizeType const &n)
{
  this->Resize({n});
}

/**
 * Constructor builds an empty Tensor pre-initialiing with zeros from a vector of dimension
 * lengths
 * @param shape   vector of lengths for each dimension
 */
template <typename T, typename C>
Tensor<T, C>::Tensor(SizeVector const &dims)
{
  Resize(dims);
}

/////////////////
/// ITERATORS ///
/////////////////

template <typename T, typename C>
typename Tensor<T, C>::IteratorType Tensor<T, C>::begin()
{
  return IteratorType(data().pointer(), size(), data().size(), height(), padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::IteratorType Tensor<T, C>::end()
{
  return IteratorType(data().pointer() + data().size(), size(), data().size(), height(),
                      padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::begin() const
{
  return ConstIteratorType(data().pointer(), size(), data().size(), height(), padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::end() const
{
  return ConstIteratorType(data().pointer() + data().size(), size(), data().size(), height(),
                           padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::cbegin() const
{
  return ConstIteratorType(data().pointer(), size(), data().size(), height(), padded_height());
}

template <typename T, typename C>
typename Tensor<T, C>::ConstIteratorType Tensor<T, C>::cend() const
{
  return ConstIteratorType(data().pointer() + data().size(), size(), data().size(), height(),
                           padded_height());
}

///////////////////////
/// VIEW EXTRACTION ///
///////////////////////

/**
 * returns a view of the entire tensor
 * @tparam T Type
 * @tparam C Container
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ViewType Tensor<T, C>::View()
{
  assert(!shape_.empty());

  SizeType N = shape_.size() - 1;
  // padded_height can be 32 bytes on AVX2, set width to 1 to avoid zero-width tensors
  SizeType width = std::max(shape_[N] * stride_[N] / padded_height_, static_cast<SizeType>(1));
  assert(width > 0);
  return TensorView<Type, ContainerType>(data_, height(), width);
}

/**
 * returns a constant view of the entire tensor
 * @tparam T Type
 * @tparam C Container
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ViewType const Tensor<T, C>::View() const
{
  assert(!shape_.empty());

  SizeType N = shape_.size() - 1;
  // padded_height can be 32 bytes on AVX2, set width to 1 to avoid zero-width tensors
  SizeType width = std::max(shape_[N] * stride_[N] / padded_height_, static_cast<SizeType>(1));
  assert(width > 0);
  return TensorView<Type, ContainerType>(data_, height(), width);
}

/**
 * returns a tensor view based on the trailing dimension
 * @tparam T Type
 * @tparam C Container
 * @param index which index of the trailing dimension to view
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ViewType Tensor<T, C>::View(SizeType index)
{
  assert(shape_.size() >= 2);

  SizeType N                = shape_.size() - 1 - 1;
  SizeType dimension_length = (N == 0 ? padded_height_ : shape_[N]);
  SizeType volume           = dimension_length * stride_[N];
  SizeType width            = volume / padded_height_;
  SizeType offset           = volume * index;
  return TensorView<Type, ContainerType>(data_, height(), width, offset);
}

template <typename T, typename C>
typename Tensor<T, C>::ViewType const Tensor<T, C>::View(SizeType index) const
{
  assert(shape_.size() >= 2);

  SizeType N                = shape_.size() - 1 - 1;
  SizeType dimension_length = (N == 0 ? padded_height_ : shape_[N]);
  SizeType volume           = dimension_length * stride_[N];
  SizeType width            = volume / padded_height_;
  SizeType offset           = volume * index;
  return TensorView<Type, ContainerType>(data_, height(), width, offset);
}

template <typename T, typename C>
typename Tensor<T, C>::ViewType Tensor<T, C>::View(std::vector<SizeType> indices)
{
  assert(shape_.size() >= 1 + indices.size());

  SizeType N                = shape_.size() - 1 - indices.size();
  SizeType dimension_length = (N == 0 ? padded_height_ : shape_[N]);
  SizeType volume           = dimension_length * stride_[N];
  SizeType width            = volume / padded_height_;
  SizeType offset           = 0;

  for (SizeType i = 0; i < indices.size(); ++i)
  {
    SizeType g = N + i + 1;
    offset += stride_[g] * indices[i];
  }

  return TensorView<Type, ContainerType>(data_, height(), width, offset);
}

template <typename T, typename C>
typename Tensor<T, C>::ViewType const Tensor<T, C>::View(std::vector<SizeType> indices) const
{
  assert(shape_.size() >= 1 + indices.size());

  SizeType N                = shape_.size() - 1 - indices.size();
  SizeType dimension_length = (N == 0 ? padded_height_ : shape_[N]);
  SizeType volume           = dimension_length * stride_[N];
  SizeType width            = volume / padded_height_;
  SizeType offset           = 0;

  for (SizeType i = 0; i < indices.size(); ++i)
  {
    SizeType g = N + i + 1;
    offset += stride_[g] * indices[i];
  }

  return TensorView<Type, ContainerType>(data_, height(), width, offset);
}

//////////////////////////////
/// ASSIGNMENT & ACCESSING ///
//////////////////////////////

/**
 * Copies input data into current array
 *
 * @param[in]     x is another Tensor of which the data, size, and shape will be copied locally.
 *
 **/
template <typename T, typename C>
void Tensor<T, C>::Copy(Tensor const &x)
{
  this->data_          = x.data_.Copy();
  this->size_          = x.size_;
  this->padded_height_ = x.padded_height_;
  this->shape_         = x.shape();
  this->stride_        = x.stride();
}

/**
 * Provides an Tensor that is a copy of the current Tensor
 *
 * @return A Tensor with the same data, size, and shape of this array.
 *
 **/
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Copy() const
{
  Tensor copy;
  copy.Copy(*this);
  return copy;
}

/**
 * Assign makes a deep copy from the data in the tensorslice into this tensor
 * @tparam T Type
 * @tparam C Container
 * @param other TensorSlice of another Tensor to assign data into this
 */
template <typename T, typename C>
void Tensor<T, C>::Assign(TensorSlice const &other)
{
  auto it1 = begin();
  auto it2 = other.cbegin();
  assert(it1.size() == it2.size());
  while (it1.is_valid())
  {
    *it1 = *it2;
    ++it1;
    ++it2;
  }
}

/**
 * Assign makes a deep copy of data from another tensor into this one
 * @tparam T Type
 * @tparam C Container
 * @param other Another Tensor to assign data from into this
 */
template <typename T, typename C>
void Tensor<T, C>::Assign(Tensor const &other)
{
  if (this->size() == other.size())
  {
    auto it1 = begin();
    auto it2 = other.cbegin();

    while (it1.is_valid())
    {
      *it1 = *it2;
      ++it1;
      ++it2;
    }
  }
  else
  {
    if (!(Broadcast(
            [](T const &x, T const &y, T &z) {
              FETCH_UNUSED(x);
              z = y;
            },
            *this, other, *this)))
    {
      throw exceptions::WrongShape("arrays not broadcastable for assignment!");
    }
  }
}

/**
 * Assign makes a deep copy of data from another tensor into this one
 * @tparam T
 * @tparam C
 * @param other Another tensorview to assign data from into this
 */
template <typename T, typename C>
void Tensor<T, C>::Assign(TensorView<Type, ContainerType> const &other)
{
  auto this_view = this->View();
  this_view.Assign(other);
}

/**
 * Operator for accessing data in the tensor
 * @tparam T Type
 * @tparam C Container
 * @tparam Indices parameter pack of indices
 * @param indices Indices for accessing the data
 * @return Returns the value in tensor at the location specified
 */
template <typename T, typename C>
typename Tensor<T, C>::Type Tensor<T, C>::operator()(SizeType const &index) const
{
  assert(index < this->size_);
  return operator[](index);
}

/**
 * Assignment operator uses iterators to assign every value in the
 * const slice of a Tensor into this tensor
 * @tparam T Type
 * @tparam C Container
 * @param slice Tensor slice of another tensor object
 * @return This tensor after assignment
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::operator=(ConstSliceType const &slice)
{
  auto it1 = begin();
  auto it2 = slice.cbegin();
  assert(it1.size() == it2.size());
  while (it1.is_valid())
  {
    *it1 = *it2;
    ++it1;
    ++it2;
  }
  return *this;
}

// TODO(ML-432)
///**
// * Assignment operator uses iterators to assign every value in the
// * slice of a Tensor into this tensor
// * @tparam T
// * @tparam C
// * @param slice
// * @return
// */
// template <typename T, typename C>
// Tensor<T, C> &Tensor<T, C>::operator=(TensorSlice const &slice)
//{
//  auto it1 = begin();
//  auto it2 = slice.begin();
//  assert(it1.size() == it2.size());
//  while (it1.is_valid())
//  {
//    *it1 = *it2;
//    ++it1;
//    ++it2;
//  }
//  return *this;
//}

/**
 * Resizes and reshapes tensor according to newly specified shape
 * @param shape the new shape to set
 * @param copy whether to copy old data to new container or not
 */
template <typename T, typename C>
bool Tensor<T, C>::Resize(SizeVector const &shape, bool copy)
{
  // if the shape is exactly the same and a copy of value is required, dont do anything
  if ((this->shape() == shape) && copy)
  {
    return true;
  }

  // a shallow copy for speedy initializion of a tensor
  Tensor   old_tensor        = *this;
  SizeType old_size          = this->size();
  SizeType new_size_unpadded = Tensor::SizeFromShape(shape);
  if (copy && (old_size == new_size_unpadded))
  {
    old_tensor = this->Copy();
  }

  SizeType new_size = Tensor::PaddedSizeFromShape(shape);
  data_             = ContainerType(new_size);
  data_.SetAllZero();
  shape_         = shape;
  size_          = new_size_unpadded;
  padded_height_ = PadValue(shape[0]);
  UpdateStrides();

  // Effectively a reshape
  if (copy && (size_ == old_size))
  {
    auto it  = begin();
    auto oit = old_tensor.begin();
    assert(it.size() == oit.size());
    while (it.is_valid())
    {
      *it = *oit;
      ++it;
      ++oit;
    }
    return true;
  }
  return false;
}

/**
 * Resizes and reshapes tensor according to newly specified shape
 * @param shape the new shape to set
 */
template <typename T, typename C>
bool Tensor<T, C>::Reshape(SizeVector const &shape)
{
  return Resize(shape, true);
}

// TODO(ML-432) - re-enable with support for small DataTypes
///**
// * Fills entire tensor with value
// * @param value
// * @param range
// */
// template <typename T, typename C>
// void Tensor<T, C>::Fill(Type const &value, memory::Range const &range)
//{
//  VectorRegisterType val(value);
//
//  this->data().in_parallel().Apply(range, [val](VectorRegisterType &z) { z = val; });
//}

/**
 * Fill entire Tensor with value
 * @param value
 */
template <typename T, typename C>
void Tensor<T, C>::Fill(Type const &value)
{
  for (auto &x : *this)
  {
    x = value;
  }
  // TODO(?): Implement all relevant vector functions
  // VectorRegisterType val(value);
  // this->data().in_parallel().Apply([val](VectorRegisterType &z) { z = val; });
}

/**
 * Sets all elements to zero
 * @tparam T Type
 * @tparam C Container
 */
template <typename T, typename C>
void Tensor<T, C>::SetAllZero()
{
  data().SetAllZero();
}

/**
 * Sets all elements to one
 * @tparam T Type
 * @tparam C Container
 */
template <typename T, typename C>
void Tensor<T, C>::SetAllOne()
{
  auto it = this->begin();
  while (it.is_valid())
  {
    *it = Type{1};
    ++it;
  }
}

/**
 * Set all padded bytes to zero.
 *
 * This method sets the padded bytes to zero. Padded bytes are those
 * which are added to ensure that the arrays true size is a multiple
 * of the vector unit.
 */
template <typename T, typename C>
void Tensor<T, C>::SetPaddedZero()
{
  data().SetPaddedZero();
}

/**
 * returns a const ref to the underlying data
 * @tparam T
 * @tparam C
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ContainerType const &Tensor<T, C>::data() const
{
  return data_;
}

template <typename T, typename C>
typename Tensor<T, C>::ContainerType &Tensor<T, C>::data()
{
  return data_;
}

/**
 * Fills the current array with a range
 * @tparam Unsigned an unsigned integer type
 * @param from starting point of range
 * @param to end of range
 * @return a reference to this
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::FillArange(Type const &from, Type const &to)
{
  Tensor ret;

  SizeType N     = this->size();
  auto     d     = static_cast<Type>(from);
  auto     delta = static_cast<Type>(static_cast<Type>(to - from) / static_cast<Type>(N));
  for (SizeType i = 0; i < N; ++i)
  {
    this->operator[](i) = Type(d);
    d                   = static_cast<Type>(d + delta);
  }
  return *this;
}

/**
 * return a tensor filled with uniform random numbers
 * @tparam T Type
 * @tparam C Container
 * @param N The new size of the tensor after filling with random
 * @return The Tensor to return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::UniformRandom(SizeType const &N)
{
  Tensor ret;
  ret.Resize({N});
  ret.FillUniformRandom();

  return ret;
}

/**
 * Returns a tensor filled with uniform random integers
 * @tparam T Type
 * @tparam C Container
 * @param N The new size after assigning value
 * @param min the minimum possible random value
 * @param max the maximum possible random value
 * @return The return Tensor filled with random values
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::UniformRandomIntegers(SizeType const &N, int64_t min, int64_t max)
{
  Tensor ret;
  ret.Resize({N});
  ret.FillUniformRandomIntegers(min, max);

  return ret;
}

/**
 * Fills tensor with uniform random data
 * @tparam T Type
 * @tparam C Container
 * @return The return Tensor filled with random valuess
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::FillUniformRandom()
{
  for (SizeType i = 0; i < this->size(); ++i)
  {
    this->operator[](i) = random::Random::generator.AsType<Type>();
  }
  return *this;
}

/**
 * Fills tensor with uniform random integers
 * @tparam T
 * @tparam C
 * @param min
 * @param max
 * @return Fills tensor with random integers
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::FillUniformRandomIntegers(int64_t min, int64_t max)
{
  assert(min <= max);

  auto diff = uint64_t(max - min);

  for (SizeType i = 0; i < this->size(); ++i)
  {
    this->operator[](i) = Type(int64_t(random::Random::generator() % diff) + min);
  }

  return *this;
}

/**
 * Method returning an Tensor of zeroes
 *
 * @param shape : a vector representing the shape of the Tensor
 * @return Tensor with all zeroes
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Zeroes(SizeVector const &shape)
{
  SizeType n = PaddedSizeFromShape(shape);
  Tensor   output{n};
  output.SetAllZero();
  output.Reshape(shape);
  return output;
}

/**
 * Method returning an Tensor of ones
 *
 * @param shape : a vector representing the shape of the Tensor
 * @return Tensor with all ones
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Ones(SizeVector const &shape)
{

  Tensor output{shape};
  output.SetAllOne();
  return output;
}

/**
 * Copmutes the single value index of a datum in tensor from a n-dim vector of indices
 * @param indices dimension indices
 * @return index in the underlying data structure
 */
template <typename T, typename C>
SizeType Tensor<T, C>::ComputeIndex(SizeVector const &indices) const
{
  return ComputeColIndex(indices);
}

////////////////////////////
/// IMPORT & EXPORT DATA ///
////////////////////////////

/**
 * This method allows Tensor instantiation from a string which is convenient for quickly writing
 * tests.
 * @tparam T Type
 * @tparam C Container
 * @param c bytearray indicating the values to fill the array with
 * @return Return Tensor with the specified values
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::FromString(byte_array::ConstByteArray const &c)
{
  Tensor            ret;
  SizeType          n = 0;
  std::vector<Type> elems;
  elems.reserve(1024);
  bool failed         = false;
  bool prev_backslash = false;
  enum
  {
    UNSET,
    SEMICOLON,
    NEWLINE
  } new_row_marker                = UNSET;
  bool        reached_actual_data = false;
  std::size_t first_row_size      = 0;
  std::size_t current_row_size    = 0;

  // Text parsing loop
  for (SizeType i = 0; i < c.size();)
  {
    SizeType last = i;
    switch (c[i])
    {
    case ';':
      if (reached_actual_data)
      {
        if (new_row_marker == UNSET)
        {
          new_row_marker = SEMICOLON;
          // The size of the first row is the size of the vector so far
          first_row_size = elems.size();
        }
        if (new_row_marker == SEMICOLON)
        {
          if ((i < c.size() - 1))
          {
            reached_actual_data = false;
            if (current_row_size != first_row_size)
            {
              // size is not a multiple of first_row_size
              std::stringstream s;
              s << "Invalid shape: row " << n << " has " << current_row_size
                << " elements, should have " << first_row_size;
              throw exceptions::WrongShape(s.str());
            }
          }
        }
      }
      ++i;
      break;
    case 'r':
    case 'n':
      if (!prev_backslash)
      {
        break;
      }
      prev_backslash = false;
      FETCH_FALLTHROUGH;  // explicit fallthrough to the next case
    case '\r':
    case '\n':
      if (reached_actual_data)
      {
        if (new_row_marker == UNSET)
        {
          new_row_marker = NEWLINE;
          // The size of the first row is the size of the vector so far
          first_row_size = elems.size();
        }
        if (new_row_marker == NEWLINE)
        {
          if ((i < c.size() - 1))
          {
            reached_actual_data = false;
            if (current_row_size != first_row_size)
            {
              // size is not a multiple of first_row_size
              std::stringstream s;
              s << "Invalid shape: row " << n << " has " << current_row_size
                << " elements, should have " << first_row_size;
              throw exceptions::WrongShape(s.str());
            }
          }
        }
      }
      ++i;
      break;
    case '\\':
      prev_backslash = true;
      ++i;
      break;
    case ',':
    case ' ':
    case '+':
    case '\t':
      prev_backslash = false;
      ++i;
      break;
    default:
      if (byte_array::consumers::NumberConsumer<1, 2>(c, i) == -1)
      {
        throw exceptions::InvalidNumericCharacter("invalid character used in string to set tensor");
      }
      else
      {
        std::string cur_elem((c.char_pointer() + last), static_cast<std::size_t>(i - last));
        elems.emplace_back(fetch::math::Type<Type>(cur_elem));
        prev_backslash = false;
        if (!reached_actual_data)
        {
          // Where we actually start counting rows
          ++n;
          reached_actual_data = true;
          current_row_size    = 0;
        }
        current_row_size++;
      }
      break;
    }
  }
  // Check last line parsed also
  if ((first_row_size > 0) && (current_row_size != first_row_size))
  {
    // size is not a multiple of first_row_size
    std::stringstream s;
    s << "Invalid shape: row " << n << " has " << current_row_size << " elements, should have "
      << first_row_size;
    throw exceptions::WrongShape(s.str());
  }

  if (n == 0)
  {
    throw exceptions::WrongShape("Shape cannot contain zeroes");
  }

  SizeType m = elems.size() / n;

  if ((m * n) != elems.size())
  {
    failed = true;
  }

  if (!failed)
  {
    ret.Resize({n, m});

    SizeType k = 0;
    for (SizeType i = 0; i < n; ++i)
    {
      for (SizeType j = 0; j < m; ++j)
      {
        ret(i, j) = elems[k++];
      }
    }
  }

  return ret;
}

/**
 * useful for printing tensor contents
 * @return
 */
template <typename T, typename C>
std::string Tensor<T, C>::ToString() const
{
  std::stringstream ss;
  ss << std::setprecision(5) << std::fixed << std::showpos;
  if (shape_.size() == 1)
  {
    for (SizeType i(0); i < shape_[0]; ++i)
    {
      ss << At(i) << ", ";
    }
  }
  else if (shape_.size() == 2)
  {
    for (SizeType i(0); i < shape_[0]; ++i)
    {
      for (SizeType j(0); j < shape_[1]; ++j)
      {
        if (j == (shape_[1] - 1))
        {
          ss << At(i, j) << ";";
        }
        else
        {
          ss << At(i, j) << ", ";
        }
      }
    }
  }
  else
  {
    throw exceptions::WrongShape("cannot convert > 2D tensors to string");
  }
  return ss.str();
}

////////////////////
/// SHAPE & SIZE ///
////////////////////

/**
 * returns the tensor's current shape
 * @return the stride of the tensor as a vector of size_type
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeVector const &Tensor<T, C>::stride() const
{
  return stride_;
}

/**
 * returns the tensor's current shape
 * @return  shape_ is the shape of the tensor as a vector of size_type
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeVector const &Tensor<T, C>::shape() const
{
  return shape_;
}

/**
 * returns the size of a specified dimension
 * @tparam T Type
 * @tparam C Container
 * @param n the dimension to query
 * @return SizeType value indicating the size of a dimension of the Tensor
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType const &Tensor<T, C>::shape(SizeType const &n) const
{
  return shape_[n];
}

/**
 * returns the size of the tensor
 * @tparam T Type
 * @tparam C Container
 * @return SizeType value indicating total size of Tensor
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType Tensor<T, C>::size() const
{
  return size_;
}

/**
 * compute tensor size based on the shape
 * @tparam T
 * @tparam C
 * @param shape
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType Tensor<T, C>::SizeFromShape(SizeVector const &shape)
{
  if (shape.empty())
  {
    return SizeType{0};
  }
  return std::accumulate(std::begin(shape), std::end(shape), SizeType{1}, std::multiplies<>());
}

template <typename T, typename C>
typename Tensor<T, C>::SizeType Tensor<T, C>::PaddedSizeFromShape(SizeVector const &shape)
{
  if (shape.empty())
  {
    return SizeType{0};
  }
  return PadValue(shape[0]) *
         std::accumulate(std::begin(shape) + 1, std::end(shape), SizeType{1}, std::multiplies<>());
}

/**
 * Flattens the array to 1 dimension
 * @tparam T
 * @tparam C
 */
template <typename T, typename C>
void Tensor<T, C>::Flatten()
{
  Reshape({size()});
}

/**
 * Instantiates a new tensor which is the transpose of this 2D tensor
 * @tparam T Type
 * @tparam C Container
 * @return Returns new transposed Tensor
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Transpose() const
{
  // TODO (private 867) -
  if (shape_.size() != 2)
  {
    throw exceptions::WrongShape("Can not transpose a tensor which is not 2-dimensional!");
  }
  SizeVector new_axes{1, 0};

  Tensor ret({shape().at(1), shape().at(0)});
  TransposeImplementation(new_axes, ret);
  return ret;
}

/**
 * Instantiates a new tensor which is the transpose of this ND tensor by the specified axes
 * @tparam T Type
 * @tparam C Container
 * @param new_axes the new order of the axes
 * @return New tensor transposed as determined by new_axes
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Transpose(SizeVector &new_axes) const
{
  assert(shape_.size() > 1);
  assert(shape_.size() == new_axes.size());

  SizeVector new_shape;
  new_shape.reserve(new_shape.size());

  for (auto &val : new_axes)
  {
    new_shape.emplace_back(shape_.at(val));
  }

  Tensor ret(new_shape);

  TransposeImplementation(new_axes, ret);
  return ret;
}

/**
 * Removes the leading dimension of the tensor if it has size 1
 * @tparam T Type
 * @tparam C Container
 * @return This tensor after squeezing
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::Squeeze()
{
  auto shape = shape_;

  bool     not_found = true;
  SizeType cur_dim   = shape.size() - 1;
  while (not_found)
  {
    if (shape.at(cur_dim) == static_cast<SizeType>(1))
    {
      shape.erase(shape.begin() + static_cast<int32_t>(cur_dim));
      Reshape(shape);
      not_found = false;
    }
    else
    {
      if (cur_dim == 0)
      {
        throw exceptions::InvalidReshape("cannot squeeze tensor, no dimensions of size 1");
      }
      --cur_dim;
    }
  }
  return *this;
}

/**
 * Adds a leading dimension of size 1
 * @tparam T Type
 * @tparam C Container
 * @return This tensor after unsqueeze
 */
template <typename T, typename C>
Tensor<T, C> &Tensor<T, C>::Unsqueeze()
{
  auto shape = shape_;
  shape.emplace_back(1);

  Reshape(shape);

  return *this;
}

///////////////////////
/// MATH OPERATIONS ///
///////////////////////

/**
 * adds two Tensors together and supports broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineAdd(Tensor const &other)
{
  Add(*this, other, *this);
  return *this;
}

/**
 * adds a scalar to every element in the array and returns the new output
 * @param scalar to add
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineAdd(Type const &scalar)
{
  Add(*this, scalar, *this);
  return *this;
}

/**
 * Subtract one Tensor from another and support broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineSubtract(Tensor const &other)
{
  Subtract(*this, other, *this);
  return *this;
}

/**
 * subtract a scalar from every element in the array and return the new output
 * @param scalar to subtract
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineSubtract(Type const &scalar)
{
  Subtract(*this, scalar, *this);
  return *this;
}

/**
 * Subtract one Tensor from another and support broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineReverseSubtract(Tensor const &other)
{
  Subtract(other, *this, *this);
  return *this;
}

/**
 * subtract every element from the scalar and return the new output
 * @param scalar to subtract
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineReverseSubtract(Type const &scalar)
{
  Subtract(scalar, *this, *this);
  return *this;
}

/**
 * multiply other tensor by this tensor and returns this
 * @tparam T Type
 * @tparam C Container
 * @param other other tensor
 * @return returns this tensor after multiplication
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineMultiply(Tensor const &other)
{
  Multiply(*this, other, *this);
  return *this;
}

/**
 * multiplies array by a scalar element wise
 * @param scalar to add
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineMultiply(Type const &scalar)
{
  Multiply(*this, scalar, *this);
  return *this;
}

/**
 * Divide Tensor by another Tensor from another and support broadcasting
 * @param other
 * @return
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineDivide(Tensor const &other)
{
  Divide(*this, other, *this);
  return *this;
}

/**
 * Divide array by a scalar elementwise
 * @param scalar to divide
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineDivide(Type const &scalar)
{
  Divide(*this, scalar, *this);
  return *this;
}

/**
 * Divide another Tensor by this Tensor from another and support broadcasting
 * @param other
 * @return this tensor after inline reverse divide
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineReverseDivide(Tensor const &other)
{
  Divide(other, *this, *this);
  return *this;
}

/**
 * Divide scalar by array elementwise
 * @param scalar to divide
 * @return new array output
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::InlineReverseDivide(Type const &scalar)
{
  Divide(scalar, *this, *this);
  return *this;
}
//////////////////////////////
/// ARRAY ORDER OPERATIONS ///
//////////////////////////////

/**
 * toggles the tensor major order; by default Tensors are column major order
 * @tparam T
 * @tparam C
 */
template <typename T, typename C>
void Tensor<T, C>::MajorOrderFlip()
{
  // it's rather strange to invoke ColumnToRow for a 1D array, but it's technically legal (all we
  // do is changed the label)
  if (this->shape().size() > 1)
  {
    if (MajorOrder() == MAJOR_ORDER::COLUMN)
    {
      FlipMajorOrder(MAJOR_ORDER::ROW);
      major_order_ = MAJOR_ORDER::ROW;
    }
    else
    {
      FlipMajorOrder(MAJOR_ORDER::COLUMN);
      major_order_ = MAJOR_ORDER::COLUMN;
    }
  }

  if (MajorOrder() == MAJOR_ORDER::COLUMN)
  {
    major_order_ = MAJOR_ORDER::ROW;
  }
  else
  {
    major_order_ = MAJOR_ORDER::COLUMN;
  }
}

/**
 * returns the current major order of the array
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::MAJOR_ORDER Tensor<T, C>::MajorOrder() const
{
  return major_order_;
}

//////////////
/// SLICES ///
//////////////

template <typename T, typename C>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::Slice() const
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({0, shape().at(j), 1});
  }

  return ConstSliceType(*this, range, 0);
}

/**
 * Returns a Slice that is not permitted to alter the original tensor
 * @tparam T
 * @tparam C
 * @param index
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::Slice(SizeType index, SizeType axis) const
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    if (axis == j)
    {
      range.push_back({index, index + 1, 1});
    }
    else
    {
      range.push_back({0, shape().at(j), 1});
    }
  }

  return ConstSliceType(*this, range, axis);
}

/**
 * Returns a Slice along multiple dimensions that is not permitted to alter the original tensor
 * @tparam T
 * @tparam C
 * @param indices
 * @param axes
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::Slice(std::vector<SizeType> indices,
                                                          std::vector<SizeType> axes) const
{
  std::vector<std::vector<SizeType>> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({0, shape().at(j), 1});
  }

  for (SizeType j = 0; j < indices.size(); ++j)
  {
    range.at(axes.at(j)).at(0) = indices.at(j);
    range.at(axes.at(j)).at(1) = indices.at(j) + 1;
    range.at(axes.at(j)).at(2) = 1;
  }

  return ConstSliceType(*this, range, axes);
}

template <typename T, typename C>
typename Tensor<T, C>::ConstSliceType Tensor<T, C>::Slice(
    std::vector<SizeType> const &begins, std::vector<SizeType> const &ends,
    std::vector<SizeType> const &strides) const
{
  std::vector<std::vector<SizeType>> range;
  std::vector<SizeType>              axis;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({begins.at(j), ends.at(j), strides.at(j)});
    axis.emplace_back(j);
  }

  return ConstSliceType(*this, range, axis);
}

template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice()
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({0, shape().at(j), 1});
  }

  return TensorSlice(*this, range, 0);
}

/**
 * Returns a Slice of the tensor
 * @tparam T
 * @tparam C
 * @param index
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice(SizeType index, SizeType axis)
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    if (axis == j)
    {
      range.push_back({index, index + 1, 1});
    }
    else
    {
      range.push_back({0, shape().at(j), 1});
    }
  }

  return TensorSlice(*this, range, axis);
}

/**
 * Returns a Slice Range of the tensor
 * @tparam T
 * @tparam C
 * @param index
 * @param axis
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice(
    std::pair<SizeType, SizeType> start_end_index, SizeType axis)
{
  std::vector<SizeVector> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    if (axis == j)
    {
      assert(start_end_index.first < start_end_index.second);
      assert(start_end_index.first >= static_cast<SizeType>(0));
      assert(start_end_index.second <= shape(j));
      range.push_back({start_end_index.first, start_end_index.second, 1});
    }
    else
    {
      range.push_back({0, shape().at(j), 1});
    }
  }

  return TensorSlice(*this, range, axis);
}

/**
 * Returns a Slice along multiple dimensions that is not permitted to alter the original tensor
 * @tparam T
 * @tparam C
 * @param indices
 * @param axes
 * @return
 */
template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice(std::vector<SizeType> indices,
                                                       std::vector<SizeType> axes)
{
  std::vector<std::vector<SizeType>> range;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({0, shape().at(j), 1});
  }

  for (SizeType j = 0; j < indices.size(); ++j)
  {
    range.at(axes.at(j)).at(0) = indices.at(j);
    range.at(axes.at(j)).at(1) = indices.at(j) + 1;
    range.at(axes.at(j)).at(2) = 1;
  }

  return TensorSlice(*this, range, axes);
}

template <typename T, typename C>
typename Tensor<T, C>::TensorSlice Tensor<T, C>::Slice(std::vector<SizeType> const &begins,
                                                       std::vector<SizeType> const &ends,
                                                       std::vector<SizeType> const &strides)
{
  std::vector<std::vector<SizeType>> range;
  std::vector<SizeType>              axis;

  for (SizeType j = 0; j < shape().size(); ++j)
  {
    range.push_back({begins.at(j), ends.at(j), strides.at(j)});
    axis.emplace_back(j);
  }

  return TensorSlice(*this, range, axis);
}

//////////////////
/// COMPARISON ///
//////////////////

template <typename T, typename C>
bool Tensor<T, C>::AllClose(Tensor const &o, Type const &relative_tolerance,
                            Type const &absolute_tolerance) const
{
  // Only enforcing number of elements
  // we allow for different shapes as long as element are in same order
  assert(o.size() == this->size());
  auto it1  = this->cbegin();
  auto eit1 = this->cend();
  auto it2  = o.cbegin();

  while (it1 != eit1)
  {
    T e1 = *it1;
    T e2 = *it2;
    ++it1;
    ++it2;

    T abs_e1;
    T abs_e2;
    T abs_diff;
    fetch::math::Abs(e1, abs_e1);
    fetch::math::Abs(e2, abs_e2);
    fetch::math::Abs(T(e1 - e2), abs_diff);
    T tolerance = fetch::vectorise::Max(
        absolute_tolerance, T(fetch::vectorise::Max(abs_e1, abs_e2) * relative_tolerance));
    if (abs_diff > tolerance)
    {
      return false;
    }
  }
  return true;
}

/**
 * Equality operator
 * This method is sensitive to height and width
 * @param other  the array which this instance is compared against
 * @return
 */
template <typename T, typename C>
bool Tensor<T, C>::operator==(Tensor<T, C> const &other) const
{
  if (shape() != other.shape())
  {
    return false;
  }
  if (size() != other.size())
  {
    return false;
  }

  bool ret      = true;
  auto it       = cbegin();
  auto other_it = other.cbegin();
  while (ret && it.is_valid())
  {
    ret &= (*it == *other_it);
    ++it;
    ++other_it;
  }

  return ret;
}

/**
 * Not-equal operator
 * This method is sensitive to height and width
 * @param other the array which this instance is compared against
 * @return
 */
template <typename T, typename C>
bool Tensor<T, C>::operator!=(Tensor<T, C> const &other) const
{
  return !(this->operator==(other));
}

/////////////////////////
/// GENERAL UTILITIES ///
/////////////////////////

/**
 * find index of value in tensor. If it's not there return max_val
 * @param val
 * @return
 */
template <typename T, typename C>
SizeType Tensor<T, C>::Find(Type val) const
{
  SizeType idx{0};
  for (auto const &cur_val : *this)
  {
    if (cur_val == val)
    {
      return idx;
    }
    ++idx;
  }
  return numeric_max<SizeType>();
}

/**
 * Concatenate tensors on the specified axis and return a new Tensor. The shape of the new tensor
 * will be identical to all tensors input except on the axis specified where the shape will be the
 * sum of tensor sizes at that axis
 * @param tensors
 * @param axis
 * @returnf
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Concat(std::vector<Tensor> const &tensors, SizeType const axis)
{
  // cant concatenate a single tensor
  assert(tensors.size() > 1);
  SizeVector tensor0_shape = tensors[0].shape();
  // specified axis must be within range of tensor axes
  assert(axis < tensor0_shape.size());

  // all tensors must have same shape except on the axis dimension
  // also we need to know the sum of axis dimensions
  SizeType sum_axis_size = 0;
  for (std::size_t i = 0; i < tensors.size(); ++i)
  {
    for (std::size_t j = 0; j < tensors[i].shape().size(); ++j)
    {
      if (j != axis)
      {
        assert(tensors[i].shape()[j] == tensor0_shape[j]);
      }
      else
      {
        sum_axis_size += tensors[i].shape()[j];
      }
    }
  }

  // set up the return tensor shape
  SizeVector ret_tensor_shape{tensor0_shape};
  ret_tensor_shape[axis] = sum_axis_size;
  Tensor ret{ret_tensor_shape};

  // copy the data across for each tensor
  SizeType                cur_from{0};
  SizeType                cur_to{0};
  std::vector<SizeVector> step{ret_tensor_shape.size()};
  std::vector<SizeType>   cur_step(3);

  cur_step[2] = 1;  // stepsize always 1 for now

  for (SizeType i{0}; i < tensors.size(); ++i)
  {
    cur_to += tensors[i].shape()[axis];

    // identify the relevant subview to fill
    for (SizeType j{0}; j < ret.shape().size(); ++j)
    {
      if (j == axis)
      {
        cur_step[0] = cur_from;
        cur_step[1] = cur_to;
        step[j]     = cur_step;
      }
      else
      {
        cur_step[0] = 0;
        cur_step[1] = ret.shape()[j];
        step[j]     = cur_step;
      }
    }

    // copy the data across
    TensorSliceIterator<T, C> ret_it{ret, step};
    auto                      t_it = tensors[i].cbegin();

    while (t_it.is_valid())
    {
      *ret_it = *t_it;
      ++ret_it;
      ++t_it;
    }

    cur_from = cur_to;
  }

  return ret;
}

/**
 * Splits a Tensor up into a vector of tensors (effectively an UnConcatenate function)
 * @param tensors
 * @param axis
 * @returnf
 */
template <typename T, typename C>
typename std::vector<Tensor<T, C>> Tensor<T, C>::Split(Tensor const &    tensor,
                                                       SizeVector const &concat_points,
                                                       SizeType const    axis)
{
  std::vector<Tensor> ret{concat_points.size()};

  // Move implementation to Tensor::UnConcatenate
  SizeType                cur_from{0};
  SizeType                cur_to{0};
  std::vector<SizeVector> step{tensor.shape().size()};
  std::vector<SizeType>   cur_step(3);
  cur_step[2] = 1;  // stepsize always 1 for now

  for (SizeType i{0}; i < ret.size(); ++i)
  {
    // extract the relevant portion of the error_signal
    cur_to += concat_points[i];

    // identify the relevant subview to fill
    for (SizeType j{0}; j < tensor.shape().size(); ++j)
    {
      if (j == axis)
      {
        cur_step[0] = cur_from;
        cur_step[1] = cur_to;
        step[j]     = cur_step;
      }
      else
      {
        cur_step[0] = 0;
        cur_step[1] = tensor.shape()[j];
        step[j]     = cur_step;
      }
    }

    // copy the data across
    ConstSliceIteratorType err_it{tensor, step};

    SizeVector cur_error_tensor_shape = tensor.shape();
    cur_error_tensor_shape[axis]      = concat_points[i];
    Tensor cur_error_tensor{cur_error_tensor_shape};

    TensorSliceIterator<T, C> t_it{cur_error_tensor};

    while (t_it.is_valid())
    {
      *t_it = *err_it;
      ++err_it;
      ++t_it;
    }

    cur_from = cur_to;

    // and assign it to the relevant return error tensor
    ret[i] = cur_error_tensor;
  }

  return ret;
}

/**
 * sorts the data into ascending order
 */
template <typename T, typename C>
void Tensor<T, C>::Sort()
{
  std::sort(data_.pointer(), data_.pointer() + data_.size());
}

/**
 * sorts the data into ascending order
 * @param range
 */
template <typename T, typename C>
void Tensor<T, C>::Sort(memory::Range const &range)
{
  std::sort(data_.pointer() + range.from(), data_.pointer() + range.to());
}

/**
 * returns a range over this array defined using signed integers (i.e. permitting backward ranges)
 * @tparam Signed a signed integer type
 * @param from starting point of range
 * @param to end of range
 * @param delta the increment to step through the range - may be negative
 * @return returns a shapeless array with the values in *this over the specified range
 */
template <typename T, typename C>
Tensor<T, C> Tensor<T, C>::Arange(T const &from, T const &to, T const &delta)
{
  assert(delta != T{0});
  assert(((from < to) && delta > T{0}) || ((from > to) && delta < T{0}));
  Tensor ret{};
  ArangeImplementation(from, to, delta, ret);
  return ret;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Tensor<std::int8_t>;
template class Tensor<std::int16_t>;
template class Tensor<std::int32_t>;
template class Tensor<std::int64_t>;
template class Tensor<std::uint8_t>;
template class Tensor<std::uint16_t>;
template class Tensor<std::uint32_t>;
template class Tensor<std::uint64_t>;
template class Tensor<float>;
template class Tensor<double>;
template class Tensor<fixed_point::fp32_t>;
template class Tensor<fixed_point::fp64_t>;
template class Tensor<fixed_point::fp128_t>;

}  // namespace math
}  // namespace fetch
