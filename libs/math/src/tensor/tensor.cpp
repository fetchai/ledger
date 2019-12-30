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

#include "math/tensor/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace math {

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

template class Tensor<std::int16_t>;
template class Tensor<std::int32_t>;
template class Tensor<std::int64_t>;
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
