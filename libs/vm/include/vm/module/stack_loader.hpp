#pragma once

namespace fetch
{
namespace vm
{

namespace details 
{

template< typename T >
struct HasResult {
  enum { value = 1 };
};

template<>
struct HasResult<void> {
  enum { value = 0 };
};


template< int N >
struct Resetter {
  static void Reset(VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     element.Reset();
     Resetter<N-1>::Reset(vm);
  }
};

template< >
struct Resetter<0> {
  static void Reset(VM *vm)
  {

  }
};

template<typename T, int N>
struct StorerClass {

  static void StoreArgument(VM *vm, T &&val)
  {
   
    WrapperClass< T >  *obj  = new WrapperClass< T >( vm->instruction_->type_id , vm, std::move(val) );
    Value &value  = vm->stack_[ vm->sp_ - N ];

    value.Reset();    
    value.SetObject(obj, vm->instruction_->type_id);   

  }
};

template<int N>
struct StorerClass<int32_t, N> {

  static void StoreArgument(VM *vm, int32_t &&val)
  {
    assert( N <= vm->sp_ );
    assert( vm->instruction_->type_id == TypeId::Int32 );

    Value &value  = vm->stack_[ vm->sp_ - N ];

    value.Reset();    
    value.variant.i32 = val;
    value.type_id = vm->instruction_->type_id;

  }
};


template<int N>
struct StorerClass<double, N> {

  static void StoreArgument(VM *vm, double &&val)
  {
    std::cout << N  << " " << vm->sp_ << std::endl;
    assert( N <= vm->sp_ );
    assert( vm->instruction_->type_id == TypeId::Float64 );

    Value &value  = vm->stack_[ vm->sp_ - N ];

    value.Reset();    
    value.variant.f64 = val;
    value.type_id = vm->instruction_->type_id;

  }
};

// TODO: Add storers for all the other basic types

template<typename T>
struct LoaderClass {

  static T LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     WrapperClass< T > *p = static_cast<WrapperClass< T > *>(element.variant.object);

     return *p->object;
  }
};

template<>
struct LoaderClass<int8_t> {
  static int8_t LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.i8;
  }
};

template<>
struct LoaderClass<int16_t> {
  static int16_t LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.i16;
  }
};

template<>
struct LoaderClass<int32_t> {
  static int32_t LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.i32;
  }
};


template<>
struct LoaderClass<int64_t> {
  static int64_t LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.i64;
  }
};



template<>
struct LoaderClass<uint8_t> {
  static uint8_t LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.ui8;
  }
};

template<>
struct LoaderClass<uint16_t> {
  static uint16_t LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.ui16;
  }
};

template<>
struct LoaderClass<uint32_t> {
  static uint32_t LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.ui32;
  }
};


template<>
struct LoaderClass<uint64_t> {
  static uint64_t LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.ui64;
  }
};

template<>
struct LoaderClass<double> {
  static double LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.f64;
  }
};


template<>
struct LoaderClass<float> {
  static float LoadArgument(int const &N, VM *vm)
  {
     Value& element = vm->stack_[vm->sp_ - N];
     return element.variant.f32;
  }
};




}
}
}