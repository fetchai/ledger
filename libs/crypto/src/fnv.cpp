#include "crypto/fnv.hpp"

namespace fetch {
namespace crypto {

std::size_t FNVLL::hashSize() const { return hash_size; }

void FNVLL::Reset() { context_ = 2166136261; }

bool FNVLL::Update(uint8_t const *data_to_hash, std::size_t const &size)
{
  for (std::size_t i = 0; i < size; ++i)
  {
    context_ = (context_ * 16777619) ^ data_to_hash[i];
  }
  return true;
}

void FNVLL::Final(uint8_t *hash, std::size_t const &size)
{
  if (size < hash_size) throw std::runtime_error("size of input buffer is smaller than hash size.");
  hash[0] = uint8_t(context_);
  hash[1] = uint8_t(context_ >> 8);
  hash[2] = uint8_t(context_ >> 16);
  hash[3] = uint8_t(context_ >> 24);
}

uint32_t FNVLL::uint_digest() const { return context_; }

const std::size_t FNVLL::hash_size{sizeof(context_type)};

FNV::FNV() : digest_(hash_size) {}

bool FNV::Update(byte_array_type const &s) { return Update(s.pointer(), s.size()); }

void FNV::Final() { Final(digest_.pointer(), digest_.size()); }

FNV::byte_array_type FNV::digest() const
{
  assert(digest_.size() == hash_size);
  return digest_;
}

std::size_t CallableFNV::operator()(fetch::byte_array::ConstByteArray const &key) const
{
  uint32_t hash = 2166136261;
  for (std::size_t i = 0; i < key.size(); ++i)
  {
    hash = (hash * 16777619) ^ key[i];
  }

  return hash;
}

}  // namespace crypto
}  // namespace fetch
