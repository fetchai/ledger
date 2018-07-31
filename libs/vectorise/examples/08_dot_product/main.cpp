#include<iostream>


typedef fetch::memory::RectangularArray<T, C, true, true> array_2d_type;


void Dot(array_2d_type const &mA, array_2d_type const &mB,
         array_2d_type &ret) 
{

  this->Resize(mA.height(), mB.height());
  std::size_t  width = mA.padded_width(); 
  
  for (std::size_t i = 0; i < mA.height(); ++i) {
    
    vector_register_type a, b;
    vector_register_iterator_type ib(mB.data().pointer(), mB.data().size());
    
    for (std::size_t j = 0; j < mB.height(); ++j) {
      std::size_t k = 0;
      type ele = 0;
            
      vector_register_iterator_type ia(
        mA.data().pointer() + i * mA.padded_width(), mA.padded_height());
      vector_register_type c(type(0));
      
      for (; k < width; k += vector_register_type::E_BLOCK_COUNT) {
        ia.Next(a);
        ib.Next(b);
        c = c + a * b;
      }
      
      for (std::size_t l = 0; l < vector_register_type::E_BLOCK_COUNT; ++l) {
        ele += first_element(c);
        c = shift_elements_right(c);
      }
      
      this->Set(i, j, ele);
    }    
  }
  

  return ret;
}


int main() 
{


  return 0;
}
