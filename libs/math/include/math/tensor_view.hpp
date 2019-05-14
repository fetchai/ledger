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


#include "math/base_types.hpp"

namespace fetch {
namespace math {
template< typename T, typename C = memory::SharedArray<T>>
class TensorView
{
public:
  using Type                       = T;
  using ContainerType              = C;  
  using IteratorType      = TensorIterator<T, ContainerType>;
  using ConstIteratorType = ConstTensorIterator<T, ContainerType>;  

  enum
  {
    LOG_PADDING = 3,
    PADDING     = static_cast<SizeType>(1) << LOG_PADDING
  };

  TensorView(ContainerType data, SizeType height, SizeType width, SizeType offset = 0)
  : data_{std::move(data)}, 
    height_{std::move(height)}, 
    width_{std::move(width)},
    offset_{std::move(offset)},
    pointer_{data_.pointer() + offset_}
  {
    padded_height_ = PadValue(height_);
  }
/*
  IteratorType      begin();
  IteratorType      end();
  ConstIteratorType begin() const;
  ConstIteratorType end() const;
  ConstIteratorType cbegin() const;
  ConstIteratorType cend() const;
*/
  Type operator()(SizeType i, SizeType j) const
  {
    return pointer_[i + j * padded_height_];
  }

  Type& operator()(SizeType i, SizeType j)
  {
    return pointer_[i + j * padded_height_];
  }

  Type operator()(SizeType i) const
  {
    return pointer_[i];
  }

  Type& operator()(SizeType i)
  {
    return pointer_[i];    
  }


  /* @breif returns the smallest number which is a multiple of PADDING and greater than or equal to
   a desired size.
   * @param size is the size to be padded.
   & @returns the padded size
   */
  static SizeType PadValue(SizeType size)
  {
    SizeType ret = SizeType(size / PADDING) * PADDING;
    if (ret < size)
    {
      ret += PADDING;
    }
    return ret;
  }

  SizeType height() const
  {
    return height_;
  }

  SizeType width() const
  {
    return width_;
  }

  SizeType padded_size() const
  {
    return data_.padded_size();
  }

  SizeType padded_height() const
  {
    return padded_height_;
  }

  constexpr SizeType padding()
  {
    return PADDING;
  }

  ContainerType const &data() const
  {
    return data_;
  }

  ContainerType &      data()
  {
    return data_;    
  }
protected:
  void SetHeight(SizeType height)
  {
    height_ = std::move(height);
    padded_height_ = PadValue(height_);
  }

  void SetWidth(SizeType width)
  {
    width_ = std::move(width);
  }

  void SetOffset(SizeType offset)
  {
    offset_ = std::move(offset);
    pointer_ = data_.pointer() + offset_;
  }

  void SetData(ContainerType data)
  {
    data_ = std::move(data);
    pointer_ = data_.pointer() + offset_;
  }  
private:
  ContainerType data_{};  
  SizeType height_{0};
  SizeType width_{0};
  SizeType offset_{0};
  Type *pointer_{nullptr};

  SizeType padded_height_{0};
};


}
}