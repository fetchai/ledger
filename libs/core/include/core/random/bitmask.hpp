#pragma once
namespace fetch {
namespace random {

template <typename W, uint8_t B = 12, bool MSBF = true>
class BitMask
{
public:
  using float_type = double;
  using word_type  = W;
  enum
  {
    BITS_OF_PRECISION = B
  };

  void SetProbability(float_type const &d)
  {
    word_type w = word_type(d * word_type(-1));
    if (d < 0) w = 0;
    if (d >= 1) w = word_type(-1);

    if (MSBF)
    {
      w ^= (w >> 1);
      w >>= 8 * sizeof(word_type) - BITS_OF_PRECISION;
      for (std::size_t i = BITS_OF_PRECISION - 1; i < BITS_OF_PRECISION; --i)
      {
        mask_[i] = -(w & 1);
        w >>= 1;
      }
    }
    else
    {
      w ^= (w << 1);
      for (std::size_t i = 0; i < BITS_OF_PRECISION; ++i)
      {
        mask_[i] = -(w & 1);
        w >>= 1;
      }
    }
  }

  word_type const &operator[](std::size_t const &n) const { return mask_[n]; }

private:
  word_type mask_[BITS_OF_PRECISION] = {0};
};
}  // namespace random
}  // namespace fetch
