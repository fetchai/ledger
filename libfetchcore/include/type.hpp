#ifndef FETCH_PYTHON_TYPE
#define FETCH_PYTHON_TYPE
namespace fetch
{
namespace base_types
{

template< typename T >
class TypedValue 
{
public:
  TypedValue(T const &t) 
  {
    value_ = t;    
  }

  TypedValue(TypedValue const &other) 
  {
    value_ = other.value;    
  }

  operator T()
  {
    return value_;
  }

 
  TypedValue const& operator=(T const &t)
  {
    value_ = t;
    return *this;    
  }  
  
  void set_value(T const &t) 
  {
    value_ = t ;
  }

  T value() const 
  {
    return value_;
  }

  T& value() 
  {
    return value_;
  }  
  
private:
  T value_;  
  
};

};

};
#endif
