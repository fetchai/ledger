#include"http/script/variant.hpp"

namespace fetch {
namespace script {

  VariantList::VariantList() : size_(0) 
  {
    pointer_ = data_.pointer();    
  }

  VariantList::VariantList(std::size_t const& size ) 
  {
    Resize(size);
  }  

  VariantList::VariantList(VariantList const& other, std::size_t offset, std::size_t size) :
    size_(size), offset_(offset), data_(other.data_)
  {
    pointer_ = data_.pointer() + offset_;
  }  

  VariantList::VariantList(VariantList const& other) :
    size_(other.size_), offset_(other.offset_), data_(other.data_), pointer_(other.pointer_)
  {
  }

  VariantList::VariantList(VariantList && other) 
  {
    std::swap(size_,other.size_);
    std::swap(offset_,other.offset_);
    std::swap(data_,other.data_);
    std::swap(pointer_,other.pointer_);
  }

VariantList const &VariantList::operator=(VariantList const& other) {
  size_ = other.size_;
  offset_ = other.offset_;
  data_ = other.data_;
  pointer_=other.pointer_;
  return *this;
}
VariantList const &VariantList::operator=(VariantList && other) {
  std::swap(size_,other.size_);
  std::swap(offset_,other.offset_);
  std::swap(data_,other.data_);
  std::swap(pointer_,other.pointer_);
  return *this;  
}

  
  Variant const& VariantList::operator[](std::size_t const &i)  const
  {
    return pointer_[i];
  }
  
Variant & VariantList::operator[](std::size_t const &i) 
{
  return pointer_[i];
}

void VariantList::Resize(std::size_t const &n) 
{
  if(size_ == n) return;
  Reserve(n);
  size_ = n;
  }

void VariantList::LazyResize(std::size_t const &n) 
{
  if(size_ == n) return;
  LazyReserve(n);
  size_ = n;
}

void VariantList::Reserve(std::size_t const &n) 
{
  if(offset_ + n < data_.size() ) return;
  
  memory::SharedArray< Variant, 16 > new_data(n);
  new_data.SetAllZero();
  
  for(std::size_t i = 0; i < size_; ++i) {
    new_data[i] = data_[i];
  }
  
  data_ = new_data;
  offset_ = 0;    
  pointer_ = data_.pointer();
}


void VariantList::LazyReserve(std::size_t const &n) 
{
  if(offset_ + n < data_.size() ) return;
  
  memory::SharedArray< Variant, 16 > new_data(n);
  new_data.SetAllZero(); 
  
  data_ = new_data;
  offset_ = 0;    
  pointer_ = data_.pointer();
}



void VariantList::SetData(VariantList const& other, std::size_t offset, std::size_t size)  {
  data_ = other.data_;
  size_ = size;
  offset_ = offset;
  pointer_ = data_.pointer() + offset;
  }

// Array accessors
Variant& Variant::operator[](std::size_t const& i) {
  assert(type_ == ARRAY);
  assert(i < size());
  return array_[i];    
}

Variant const& Variant::operator[](std::size_t const& i) const {
  return array_[i];
}
  
  
std::size_t Variant::size() const {
  if (type_ == ARRAY)
    return array_.size();
  if (type_ == STRING) return string_.size();
  return 0;
}


  
}
}
