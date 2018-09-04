namespace fetch
{
namespace vm
{

template< typename T >
struct WrapperClass : public Object
{
  using Object::Object;
  WrapperClass( TypeId type_id, VM *vm, T &&o )
    : Object(std::move(type_id), vm), object(std::move(o))
    { }
  T object;

  virtual ~WrapperClass() = default;
};

}
}