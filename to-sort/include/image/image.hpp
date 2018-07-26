#pragma once
#include "image/load_png.hpp"
#include "math/linalg/matrix.hpp"

namespace fetch {
namespace image {

namespace colors {
template <typename V, std::size_t B, std::size_t C>
class AbstractColor {
 public:
  typedef V container_type;

  AbstractColor(container_type const &v) : value_(v) {}

  enum {
    E_CHANNELS = C,
    E_BITS_PER_CHANNEL = B,
    E_CHANNEL_MASK = (1ull << E_BITS_PER_CHANNEL) - 1
  };
  operator container_type() { return value_; }

  container_type operator[](std::size_t const &n) const {
    assert(n < E_CHANNELS);
    return (value_ >> (n * E_BITS_PER_CHANNEL)) & E_CHANNEL_MASK;
  }

 private:
  container_type value_;
};

typedef AbstractColor<uint32_t, 8, 3> RGB8;
typedef AbstractColor<uint32_t, 8, 4> RGBA8;
}

template <typename T = colors::RGBA8>
class ImageType
    : public fetch::math::linalg::Matrix<typename T::container_type> {
 public:
  typedef T color_type;
  typedef typename color_type::container_type type;
  typedef fetch::math::linalg::Matrix<type> super_type;

  enum {
    E_CHANNELS = color_type::E_CHANNELS,
    E_BITS_PER_CHANNEL = color_type::E_BITS_PER_CHANNEL,
    E_CHANNEL_MASK = (1ull << E_BITS_PER_CHANNEL) - 1
  };

  ImageType() = default;
  ImageType(ImageType &&other) = default;
  ImageType(ImageType const &other) = default;
  ImageType &operator=(ImageType const &other) = default;
  ImageType &operator=(ImageType &&other) = default;

  ImageType(super_type const &other) : super_type(other) {}
  ImageType(super_type &&other) : super_type(other) {}
  ImageType(std::size_t const &h, std::size_t const &w) : super_type(h, w) {}

  void Load(std::string const &file) { LoadPNG(file, *this); }
};

typedef ImageType<colors::RGB8> ImageRGB;
typedef ImageType<colors::RGBA8> ImageRGBA;
}
}

