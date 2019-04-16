#include <iostream>
#include <vector>

using SizeVector = std::vector<uint64_t>;

template <uint64_t N, typename T, typename... Args>
struct TensorSetter
{
  // Finding the return value
  using Type = typename TensorSetter<N + 1, Args...>::Type;

  // Computing index
  static uint64_t IndexOf(SizeVector const &stride, SizeVector const &shape, T const &index,
                          Args &&... args)
  {
    assert(index < shape[N]);
    return stride[N] * index +
           TensorSetter<N + 1, Args...>::IndexOf(stride, shape, std::forward<Args>(args)...);
  }

  // Ignoring all arguments but the last
  static Type ValueOf(T const &index, Args &&... args)
  {
    return TensorSetter<N + 1, Args...>::ValueOf(std::forward<Args>(args)...);
  }
};

template <uint64_t N, typename T>
struct TensorSetter<N, T>
{
  using Type = T;

  // Ignoring last argument
  static uint64_t IndexOf(SizeVector const &stride, SizeVector const &shape, T const &index)
  {
    assert(shape.size() == N);
    assert(stride.size() == N);
    return 0;
  }

  // Extracting last argument
  static T ValueOf(T const &value)
  {
    return value;
  }
};

class Tensor
{
public:
  using Type = double;
  Tensor(SizeVector shape)
    : shape_{std::move(shape)}
  {
    stride_.reserve(shape_.size());
    uint64_t b = 1;

    for (uint64_t i = 0; i < shape_.size(); ++i)
    {
      stride_.push_back(b);
      b *= shape_[i];
    }

    data_.resize(b);
    for (auto &d : data_)
    {
      d = 0;
    }
  }

  template <typename... Args>
  void Set(Args... args)
  {
    assert(sizeof...(args) == stride_.size() + 1);  // Plus one as last arg is value

    uint64_t index =
        TensorSetter<0, Args...>::IndexOf(stride_, shape_, std::forward<Args>(args)...);
    Type value = TensorSetter<0, Args...>::ValueOf(std::forward<Args>(args)...);

    data_[std::move(index)] = std::move(value);
  }

  template <typename... Args>
  Type Get(Args... args)
  {
    uint64_t index =
        TensorSetter<0, Args..., int>::IndexOf(stride_, shape_, std::forward<Args>(args)..., 0);
    return data_[std::move(index)];
  }

private:
  SizeVector        shape_;
  SizeVector        stride_;
  std::vector<Type> data_;
};

int main()
{
  Tensor test({2, 3, 4});
  test.Set(1, 2, 1, 3.2);
  std::cout << test.Get(1, 2, 1) << std::endl;
  return 0;
}