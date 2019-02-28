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

#include "meta/type_util.hpp"
#include "vm/common.hpp"
#include <cmath>

namespace fetch {
namespace vm {

// Forward declarations
class Object;
template <typename T>
class Ptr;
struct Variant;
class VM;

template <typename T, typename = void>
struct IsPrimitive : std::false_type
{
};
template <typename T>
struct IsPrimitive<T, typename std::enable_if_t<
                          type_util::IsAnyOfV<T, void, bool, int8_t, uint8_t, int16_t, uint16_t,
                                              int32_t, uint32_t, int64_t, uint64_t, float, double>>>
  : std::true_type
{
};

template <typename T, typename = void>
struct IsObject : std::false_type
{
};
template <typename T>
struct IsObject<T, typename std::enable_if_t<std::is_base_of<Object, T>::value>> : std::true_type
{
};

template <typename T>
struct IsPtr : std::false_type
{
};
template <typename T>
struct IsPtr<Ptr<T>> : std::true_type
{
};

template <typename T, typename = void>
struct IsVariant : std::false_type
{
};
template <typename T>
struct IsVariant<T, typename std::enable_if_t<std::is_base_of<Variant, T>::value>> : std::true_type
{
};

template <typename T, typename = void>
struct IsNonconstRef : std::false_type
{
};
template <typename T>
struct IsNonconstRef<
    T, typename std::enable_if_t<std::is_same<T, typename std::decay<T>::type &>::value>>
  : std::true_type
{
};

template <typename T, typename = void>
struct IsConstRef : std::false_type
{
};
template <typename T>
struct IsConstRef<
    T, typename std::enable_if_t<std::is_same<T, typename std::decay<T>::type const &>::value>>
  : std::true_type
{
};

template <typename T>
struct GetManagedType;
template <typename T>
struct GetManagedType<Ptr<T>>
{
  using type = T;
};

template <typename T, typename = void>
struct IsPrimitiveParameter : std::false_type
{
};
template <typename T>
struct IsPrimitiveParameter<
    T, typename std::enable_if_t<!IsNonconstRef<T>::value &&
                                 IsPrimitive<typename std::decay<T>::type>::value>> : std::true_type
{
};

template <typename T, typename = void>
struct IsPtrParameter : std::false_type
{
};
template <typename T>
struct IsPtrParameter<T, typename std::enable_if_t<IsConstRef<T>::value &&
                                                   IsPtr<typename std::decay<T>::type>::value>>
  : std::true_type
{
};

template <typename T, typename = void>
struct IsVariantParameter : std::false_type
{
};
template <typename T>
struct IsVariantParameter<T,
                          typename std::enable_if_t<IsConstRef<T>::value &&
                                                    IsVariant<typename std::decay<T>::type>::value>>
  : std::true_type
{
};

template <typename T, typename = void>
struct GetStorageType;
template <typename T>
struct GetStorageType<T, typename std::enable_if_t<IsPrimitive<T>::value>>
{
  using type = T;
};
template <typename T>
struct GetStorageType<T, typename std::enable_if_t<IsPtr<T>::value>>
{
  using type = Ptr<Object>;
};

class Object
{
public:
  Object()          = delete;
  virtual ~Object() = default;

  Object(VM *vm, TypeId type_id)
  {
    vm_        = vm;
    type_id_   = type_id;
    ref_count_ = 1;
  }

  virtual size_t GetHashCode();
  virtual bool   IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void   UnaryMinus(Ptr<Object> &object);
  virtual void   Add(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   LeftAdd(Variant &lhsv, Variant &rhsv);
  virtual void   RightAdd(Variant &lhsv, Variant &rhsv);
  virtual void   AddAssign(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   RightAddAssign(Ptr<Object> &lhso, Variant &rhsv);
  virtual void   Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   LeftSubtract(Variant &lhsv, Variant &rhsv);
  virtual void   RightSubtract(Variant &lhsv, Variant &rhsv);
  virtual void   SubtractAssign(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   RightSubtractAssign(Ptr<Object> &lhso, Variant &rhsv);
  virtual void   Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   LeftMultiply(Variant &lhsv, Variant &rhsv);
  virtual void   RightMultiply(Variant &lhsv, Variant &rhsv);
  virtual void   MultiplyAssign(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   RightMultiplyAssign(Ptr<Object> &lhso, Variant &rhsv);
  virtual void   Divide(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   LeftDivide(Variant &lhsv, Variant &rhsv);
  virtual void   RightDivide(Variant &lhsv, Variant &rhsv);
  virtual void   DivideAssign(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   RightDivideAssign(Ptr<Object> &lhso, Variant &rhsv);
  virtual void * FindElement();
  virtual void   PushElement(TypeId element_type_id);
  virtual void   PopToElement();

protected:
  Variant &       Push();
  Variant &       Pop();
  Variant &       Top();
  void            RuntimeError(std::string const &message);
  TypeInfo const &GetTypeInfo(TypeId type_id);
  bool            GetNonNegativeInteger(Variant const &v, size_t &index);

  VM *   vm_;
  TypeId type_id_;
  size_t ref_count_;

private:
  void AddRef()
  {
    ++ref_count_;
  }

  void Release()
  {
    if (--ref_count_ == 0)
    {
      delete this;
    }
  }

  template <typename T>
  friend class Ptr;
};

template <typename T>
class Ptr
{
public:
  Ptr()
  {
    ptr_ = nullptr;
  }

  Ptr(T *other)
  {
    ptr_ = other;
  }

  static Ptr PtrFromThis(T *this__)
  {
    this__->AddRef();
    return Ptr(this__);
  }

  Ptr &operator=(std::nullptr_t /* other */)
  {
    Reset();
    return *this;
  }

  Ptr(Ptr const &other)
  {
    ptr_ = other.ptr_;
    AddRef();
  }

  Ptr(Ptr &&other)
  {
    ptr_       = other.ptr_;
    other.ptr_ = nullptr;
  }

  template <typename U>
  Ptr(Ptr<U> const &other)
  {
    ptr_ = static_cast<T *>(other.ptr_);
    AddRef();
  }

  template <typename U>
  Ptr(Ptr<U> &&other)
  {
    ptr_       = static_cast<T *>(other.ptr_);
    other.ptr_ = nullptr;
  }

  Ptr &operator=(Ptr const &other)
  {
    if (ptr_ != other.ptr_)
    {
      Release();
      ptr_ = other.ptr_;
      AddRef();
    }
    return *this;
  }

  Ptr &operator=(Ptr &&other)
  {
    if (this != &other)
    {
      Release();
      ptr_       = other.ptr_;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  template <typename U>
  Ptr &operator=(Ptr<U> const &other)
  {
    if (ptr_ != other.ptr_)
    {
      Release();
      ptr_ = static_cast<T *>(other.ptr_);
      AddRef();
    }
    return *this;
  }

  template <typename U>
  Ptr &operator=(Ptr<U> &&other)
  {
    Release();
    ptr_       = static_cast<T *>(other.ptr_);
    other.ptr_ = nullptr;
    return *this;
  }

  ~Ptr()
  {
    Release();
  }

  void Reset()
  {
    if (ptr_)
    {
      ptr_->Release();
      ptr_ = nullptr;
    }
  }

  explicit operator bool() const
  {
    return ptr_ != nullptr;
  }

  T *operator->() const
  {
    return ptr_;
  }

  T &operator*() const
  {
    return *ptr_;
  }

  size_t RefCount() const
  {
    return ptr_->ref_count_;
  }

private:
  T *ptr_;

  void AddRef()
  {
    if (ptr_)
    {
      ptr_->AddRef();
    }
  }

  void Release()
  {
    if (ptr_)
    {
      ptr_->Release();
    }
  }

  template <typename U>
  friend class Ptr;

  template <typename L, typename R>
  friend bool operator==(Ptr<L> const &lhs, Ptr<R> const &rhs);

  template <typename L>
  friend bool operator==(Ptr<L> const &lhs, std::nullptr_t /* rhs */);

  template <typename R>
  friend bool operator==(std::nullptr_t /* lhs */, Ptr<R> const &rhs);

  template <typename L, typename R>
  friend bool operator!=(Ptr<L> const &lhs, Ptr<R> const &rhs);

  template <typename L>
  friend bool operator!=(Ptr<L> const &lhs, std::nullptr_t /* rhs */);

  template <typename R>
  friend bool operator!=(std::nullptr_t /* lhs */, Ptr<R> const &rhs);
};

template <typename L, typename R>
inline bool operator==(Ptr<L> const &lhs, Ptr<R> const &rhs)
{
  return (lhs.ptr_ == static_cast<L *>(rhs.ptr_));
}

template <typename L>
inline bool operator==(Ptr<L> const &lhs, std::nullptr_t /* rhs */)
{
  return (lhs.ptr_ == nullptr);
}

template <typename R>
inline bool operator==(std::nullptr_t /* lhs */, Ptr<R> const &rhs)
{
  return (nullptr == rhs.ptr_);
}

template <typename L, typename R>
inline bool operator!=(Ptr<L> const &lhs, Ptr<R> const &rhs)
{
  return (lhs.ptr_ != static_cast<L *>(rhs.ptr_));
}

template <typename L>
inline bool operator!=(Ptr<L> const &lhs, std::nullptr_t /* rhs */)
{
  return (lhs.ptr_ != nullptr);
}

template <typename R>
inline bool operator!=(std::nullptr_t /* lhs */, Ptr<R> const &rhs)
{
  return (nullptr != rhs.ptr_);
}

union Primitive
{
  int8_t   i8;
  uint8_t  ui8;
  int16_t  i16;
  uint16_t ui16;
  int32_t  i32;
  uint32_t ui32;
  int64_t  i64;
  uint64_t ui64;
  float    f32;
  double   f64;

  void Zero()
  {
    ui64 = 0;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, bool>::value, T> Get() const
  {
    return bool(ui8);
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, int8_t>::value, T> Get() const
  {
    return i8;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, uint8_t>::value, T> Get() const
  {
    return ui8;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, int16_t>::value, T> Get() const
  {
    return i16;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, uint16_t>::value, T> Get() const
  {
    return ui16;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, int32_t>::value, T> Get() const
  {
    return i32;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, uint32_t>::value, T> Get() const
  {
    return ui32;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, int64_t>::value, T> Get() const
  {
    return i64;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, uint64_t>::value, T> Get() const
  {
    return ui64;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, float>::value, T> Get() const
  {
    return f32;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, double>::value, T> Get() const
  {
    return f64;
  }

  void Set(bool value)
  {
    ui8 = uint8_t(value);
  }

  void Set(int8_t value)
  {
    i8 = value;
  }

  void Set(uint8_t value)
  {
    ui8 = value;
  }

  void Set(int16_t value)
  {
    i16 = value;
  }

  void Set(uint16_t value)
  {
    ui16 = value;
  }

  void Set(int32_t value)
  {
    i32 = value;
  }

  void Set(uint32_t value)
  {
    ui32 = value;
  }

  void Set(int64_t value)
  {
    i64 = value;
  }

  void Set(uint64_t value)
  {
    ui64 = value;
  }

  void Set(float value)
  {
    f32 = value;
  }

  void Set(double value)
  {
    f64 = value;
  }
};

struct Variant
{
  union
  {
    Primitive   primitive;
    Ptr<Object> object;
  };
  TypeId type_id;

  Variant()
  {
    Construct();
  }

  Variant(Variant const &other)
  {
    Construct(other);
  }

  Variant(Variant &&other)
  {
    Construct(std::move(other));
  }

  template <typename T, typename std::enable_if_t<IsPrimitive<T>::value> * = nullptr>
  Variant(T other, TypeId other_type_id)
  {
    Construct(other, other_type_id);
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  Variant(T const &other, TypeId other_type_id)
  {
    Construct(other, other_type_id);
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  Variant(T &&other, TypeId other_type_id)
  {
    Construct(std::forward<T>(other), other_type_id);
  }

  Variant(Primitive other, TypeId other_type_id)
  {
    Construct(other, other_type_id);
  }

  void Construct()
  {
    type_id = TypeIds::Unknown;
  }

  void Construct(Variant const &other)
  {
    type_id = other.type_id;
    if (IsObject())
    {
      new (&object) Ptr<Object>(other.object);
    }
    else
    {
      primitive = other.primitive;
    }
  }

  void Construct(Variant &&other)
  {
    type_id = other.type_id;
    if (IsObject())
    {
      new (&object) Ptr<Object>(std::move(other.object));
    }
    else
    {
      primitive = other.primitive;
    }
    other.type_id = TypeIds::Unknown;
  }

  template <typename T, typename std::enable_if_t<IsPrimitive<T>::value> * = nullptr>
  void Construct(T other, TypeId other_type_id)
  {
    primitive.Set(other);
    type_id = other_type_id;
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  void Construct(T const &other, TypeId other_type_id)
  {
    new (&object) Ptr<Object>(other);
    type_id = other_type_id;
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  void Construct(T &&other, TypeId other_type_id)
  {
    new (&object) Ptr<Object>(std::forward<T>(other));
    type_id = other_type_id;
  }

  void Construct(Primitive other, TypeId other_type_id)
  {
    primitive = other;
    type_id   = other_type_id;
  }

  Variant &operator=(Variant const &other)
  {
    if (this != &other)
    {
      bool const is_object       = IsObject();
      bool const other_is_object = other.IsObject();
      type_id                    = other.type_id;
      if (is_object)
      {
        if (other_is_object)
        {
          // Copy object to current object
          object = other.object;
        }
        else
        {
          // Copy primitive to current object
          object.Reset();
          primitive = other.primitive;
        }
      }
      else
      {
        if (other_is_object)
        {
          // Copy object to current primitive
          new (&object) Ptr<Object>(other.object);
        }
        else
        {
          // Copy primitive to current primitive
          primitive = other.primitive;
        }
      }
    }
    return *this;
  }

  Variant &operator=(Variant &&other)
  {
    if (this != &other)
    {
      bool const is_object       = IsObject();
      bool const other_is_object = other.IsObject();
      type_id                    = other.type_id;
      other.type_id              = TypeIds::Unknown;
      if (is_object)
      {
        if (other_is_object)
        {
          // Move object to current object
          object = std::move(other.object);
        }
        else
        {
          // Move primitive to current object
          object.Reset();
          primitive = other.primitive;
        }
      }
      else
      {
        if (other_is_object)
        {
          // Move object to current primitive
          new (&object) Ptr<Object>(std::move(other.object));
        }
        else
        {
          // Move primitive to current primitive
          primitive = other.primitive;
        }
      }
    }
    return *this;
  }

  template <typename T, typename std::enable_if_t<IsPrimitive<T>::value> * = nullptr>
  void Assign(T other, TypeId other_type_id)
  {
    if (IsObject())
    {
      object.Reset();
    }
    primitive.Set(other);
    type_id = other_type_id;
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  void Assign(T const &other, TypeId other_type_id)
  {
    if (IsObject())
    {
      object  = other;
      type_id = other_type_id;
    }
    else
    {
      Construct(other, other_type_id);
    }
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  void Assign(T &&other, TypeId other_type_id)
  {
    if (IsObject())
    {
      object  = std::forward<T>(other);
      type_id = other_type_id;
    }
    else
    {
      Construct(std::forward<T>(other), other_type_id);
    }
  }

  template <typename T, typename std::enable_if_t<IsVariant<T>::value> * = nullptr>
  void Assign(T const &other, TypeId /* other_type_id */)
  {
    operator=(other);
  }

  template <typename T, typename std::enable_if_t<IsVariant<T>::value> * = nullptr>
  void Assign(T &&other, TypeId /* other_type_id */)
  {
    operator=(std::forward<T>(other));
  }

  template <typename T>
  typename std::enable_if_t<IsPrimitive<T>::value, T> Get() const
  {
    return primitive.Get<T>();
  }

  template <typename T>
  typename std::enable_if_t<IsPtr<T>::value, T> Get() const
  {
    return object;
  }

  template <typename T>
  typename std::enable_if_t<IsVariant<T>::value, T> Get() const
  {
    T variant;
    variant.type_id = type_id;
    if (IsObject())
    {
      new (&variant.object) Ptr<Object>(object);
    }
    else
    {
      variant.primitive = primitive;
    }
    return variant;
  }

  template <typename T>
  typename std::enable_if_t<IsPrimitive<T>::value, T> Move()
  {
    type_id = TypeIds::Unknown;
    return primitive.Get<T>();
  }

  template <typename T>
  typename std::enable_if_t<IsPtr<T>::value, T> Move()
  {
    type_id = TypeIds::Unknown;
    return std::move(object);
  }

  template <typename T>
  typename std::enable_if_t<IsVariant<T>::value, T> Move()
  {
    T variant;
    variant.type_id = type_id;
    if (IsObject())
    {
      new (&variant.object) Ptr<Object>(std::move(object));
    }
    else
    {
      variant.primitive = primitive;
    }
    type_id = TypeIds::Unknown;
    return variant;
  }

  ~Variant()
  {
    Reset();
  }

  bool IsObject() const
  {
    return type_id >= TypeIds::ObjectMinId;
  }

  void Reset()
  {
    if (IsObject())
    {
      object.Reset();
    }
    type_id = TypeIds::Unknown;
  }
};

struct TemplateParameter : public Variant
{
  using Variant::Variant;
};

struct TemplateParameter1 : public Variant
{
  using Variant::Variant;
};

struct TemplateParameter2 : public Variant
{
  using Variant::Variant;
};

struct Script
{
  Script() = default;
  Script(std::string const &name__, TypeInfoTable const &type_info_table__)
  {
    name            = name__;
    type_info_table = type_info_table__;
  }

  struct Instruction
  {
    Instruction(Opcode opcode__, uint16_t line__)
    {
      opcode  = opcode__;
      line    = line__;
      index   = 0;
      type_id = TypeIds::Unknown;
      data.Zero();
    }
    Opcode    opcode;
    uint16_t  line;
    Index     index;  // index of variable, or index into instructions (pc)
    TypeId    type_id;
    Primitive data;
  };

  using Instructions = std::vector<Instruction>;

  struct Variable
  {
    Variable(std::string const &name__, TypeId type_id__)
    {
      name    = name__;
      type_id = type_id__;
    }
    std::string name;
    TypeId      type_id;
  };

  using Variables = std::vector<Variable>;

  enum class AnnotationLiteralType : uint16_t
  {
    Unknown = 0,
    Boolean,
    Integer,
    Real,
    String,
    Identifier
  };

  struct AnnotationLiteral
  {
    AnnotationLiteral()
    {
      type = AnnotationLiteralType::Unknown;
    }
    void SetBoolean(bool b)
    {
      type    = AnnotationLiteralType::Boolean;
      boolean = b;
    }
    void SetInteger(int64_t i)
    {
      type    = AnnotationLiteralType::Integer;
      integer = i;
    }
    void SetReal(double r)
    {
      type = AnnotationLiteralType::Real;
      real = r;
    }
    void SetString(std::string const &s)
    {
      type = AnnotationLiteralType::String;
      str  = s;
    }
    void SetIdentifier(std::string const &s)
    {
      type = AnnotationLiteralType::Identifier;
      str  = s;
    }
    AnnotationLiteralType type;
    union
    {
      bool    boolean;
      int64_t integer;
      double  real;
    };
    std::string str;
  };

  enum class AnnotationElementType : uint16_t
  {
    Unknown = 0,
    Value,
    NameValuePair
  };

  struct AnnotationElement
  {
    AnnotationElement()
    {
      type = AnnotationElementType::Unknown;
    }
    AnnotationElementType type;
    AnnotationLiteral     name;
    AnnotationLiteral     value;
  };

  using AnnotationElements = std::vector<AnnotationElement>;

  struct Annotation
  {
    std::string        name;
    AnnotationElements elements;
  };

  using Annotations = std::vector<Annotation>;

  struct Function
  {
    Function(std::string const &name__, Annotations const &annotations__, int num_parameters__,
             TypeId return_type_id__)
    {
      name           = name__;
      annotations    = annotations__;
      num_variables  = 0;
      num_parameters = num_parameters__;
      return_type_id = return_type_id__;
    }
    Index AddVariable(std::string const &name, TypeId type_id)
    {
      Index const index = (Index)num_variables++;
      variables.push_back(Variable(name, type_id));
      return index;
    }
    Index AddInstruction(Instruction &instruction)
    {
      Index const pc = (Index)instructions.size();
      instructions.push_back(std::move(instruction));
      return pc;
    }
    std::string  name;
    Annotations  annotations;
    int          num_variables;  // parameters + locals
    int          num_parameters;
    TypeId       return_type_id;
    Variables    variables;  // parameters + locals
    Instructions instructions;
  };

  using Functions = std::vector<Function>;

  std::string                            name;
  TypeInfoTable                          type_info_table;
  Strings                                strings;
  Functions                              functions;
  std::unordered_map<std::string, Index> map;

  Index AddFunction(Function &function)
  {
    Index const index  = (Index)functions.size();
    map[function.name] = index;
    functions.push_back(std::move(function));
    return index;
  }

  Function const *FindFunction(std::string const &name) const
  {
    auto it = map.find(name);
    if (it != map.end())
    {
      return &(functions[it->second]);
    }
    return nullptr;
  }

  std::vector<std::string> GetFunctions() const
  {
    std::vector<std::string> ret;

    for (auto const &function : functions)
    {
      ret.push_back(function.name);
    }

    return ret;
  }
};

}  // namespace vm
}  // namespace fetch
