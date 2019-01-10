
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

template <class T>
class Tensor // Using name Tensor to not clash with current NDArray
{
public:
  Tensor(std::vector<size_t> const &shape = std::vector<size_t>(), std::vector<size_t> const &strides = std::vector<size_t>(), std::vector<size_t> const &padding = std::vector<size_t>())
    :shape_(shape), padding_(padding), strides_(strides)
  {
    assert(padding.empty() || padding.size() == shape.size());
    assert(strides.empty() || strides.size() == shape.size());
    if (!shape_.empty())
      {
	if (strides.empty())
	  {
	    strides_ = std::vector<size_t>(shape_.size(), 1);
	  }
	if (padding.empty())
	  {
	    padding_ = std::vector<size_t>(shape_.size(), 0);
	    padding_.back() = 8 - ((strides_.back() * shape_.back()) % 8); // Let's align everything on 8
	  }
	storage_ = std::make_shared<std::vector<T>>(Size());
      }
  }

  size_t NumberOfElements() const
  {
    if (shape_.empty())
      {
        return 0;
      }
    size_t n(1);
    for (size_t d : shape_)
      {
        n *= d;
      }
    return n;
  }
 
  size_t DimensionSize(size_t dim) const
  {    
    if (!shape_.empty() && dim >= 0 && dim < shape_.size())
      {
	return DimensionSizeImplementation(dim);
      }
    return 0;
  }
 
  size_t Size() const
  {
    if (shape_.empty())
      {
	return 0;
      }
    return DimensionSize(0) * shape_[0] + padding_[0];
  }

  size_t OffsetOfElement(std::vector<size_t> const &indices) const
  {
    size_t index(0);
    for (size_t i(0) ; i < indices.size() ; ++i)
      {
        assert(indices[i] < shape_[i]);
        index += indices[i] * DimensionSize(i);
      }
    return index;
  }

  
  T Get(std::vector<size_t> const &indices) const
  {
    return (*storage_)[OffsetOfElement(indices)];
  }

  void Set(std::vector<size_t> const &indices, T value)
  {
    (*storage_)[OffsetOfElement(indices)] = value;
  }

  std::shared_ptr<const std::vector<T>> Storage() const
  {
    return storage_;
  }
  
private:
  size_t DimensionSizeImplementation(size_t dim) const
  {
    if (dim == shape_.size() - 1)
      {
	return strides_[dim];
      }
    return (DimensionSizeImplementation(dim + 1) * shape_[dim + 1]  + padding_[dim + 1]) * strides_[dim];
  }
  
private:
  std::shared_ptr<std::vector<T>> storage_;
  std::vector<size_t> shape_;
  std::vector<size_t> padding_;
  std::vector<size_t> strides_;
};
