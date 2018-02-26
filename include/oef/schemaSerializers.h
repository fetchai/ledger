#include"oef/schema.h"

// all ser/deser methods
template< typename T>
void Serialize( T & serializer, Instance const &b) {

  // Put in the serializer the size of our data
  serializer << b.getValues().size();

  for(auto i : b.getValues()){
    const std::string key = i.first;
    const std::string val = i.second;
    serializer << key << val;
  }

  // let's whack on our datamodel
  serializer << b.getDataModel();
}

template< typename T>
void Deserialize( T & serializer, Instance &b) {

  uint64_t mapLen; // TODO: (`HUT`) : change sizing
  serializer >> mapLen;
  auto &map = b.setValues();
  for (int i = 0;i < mapLen;i++){
    std::string first;
    std::string second;
    serializer >> first >> second;
    map[first] = second;
  }

  // let's whack on our datamodel
  serializer >> b.setDataModel();
}

// TODO: (`HUT`) : keywords ser/deser not tested yet
template< typename T>
void Serialize( T & serializer, DataModel const &b) {
  serializer << b.getName() << b.getKeywords() << b.getAttributes();
}

template< typename T>
void Deserialize( T & serializer, DataModel &b) {
  serializer >> b.setName() << b.setKeywords() << b.setAttributes();
}

template< typename T>
void Serialize( T & serializer, Attribute const &b) {
  serializer << b.getName() << b.getRequired() << b.getType();
}

template< typename T>
void Deserialize( T & serializer, Attribute &b) {
  serializer >> b.setName() >> b.setRequired() >> b.setType();
}

// TODO: (`HUT`) : ask troells, almost certainly a more efficient way to do this using serializer type inference
template< typename T>
void Serialize( T & serializer, Type const &b) {

  switch(b) {
  case Type::Float:
    serializer << (uint32_t)1;
    break;
  case Type::Int:
    serializer << (uint32_t)2;
    break;
  case Type::Bool:
    serializer << (uint32_t)3;
    break;
  case Type::String:
    serializer << (uint32_t)4;
    break;
  default:
    serializer << (uint32_t)0;
    break;
  }
}

template< typename T>
void Deserialize( T & serializer, Type &b) {
  uint32_t res;
  serializer >> res;
  switch(res) {
  case 1:
    b= Type::Float;
    break;
  case 2:
    b= Type::Int;
    break;
  case 3:
    b= Type::Bool;
    break;
  case 4:
    b= Type::String;
    break;
  default:
    b= Type::Int;
    break;
  }
}

// QueryModel
template< typename T>
void Serialize( T & serializer, QueryModel const &b) {
  serializer << b.getConstraints();
}

template< typename T>
void Deserialize( T & serializer, QueryModel &b) {
  serializer >> b.setConstraints();
}

// QueryModel has constraints
template< typename T>
void Serialize( T & serializer, Constraint const &b) {
  serializer << b.getAttribute() << b.getConstraintType();
}

template< typename T>
void Deserialize( T & serializer, Constraint &b) {
  serializer >> b.setAttribute() >> b.setConstraintType();
}

// Constraints have an attribute(done) and a constraint type
template< typename T>
void Serialize( T & serializer, ConstraintType const &b) {
  serializer << b.getConstraint();
}

template< typename T>
void Deserialize( T & serializer, ConstraintType &b) {
  serializer >> b.setConstraint();
}

// ConstraintType
// Do we really want to do this? TODO: (`HUT`) : Ask Troells how to compress
template< typename T, typename A, typename B, typename C, typename D, typename E>
void Serialize( T & serializer, var::variant<A, B, C, D, E> const &b) {

  b.match(
          [&] (A a)        { /*serializer << a;*/ },
          [&] (B a)        { /*serializer << a;*/ },
          [&] (C a)        { /*serializer << a;*/ },
          [&] (Relation a) { serializer << a; }, // TODO: (`HUT`) : the rest of them
          [&] (E a)        { /*serializer << a;*/ }
          );
}

// TODO: (`HUT`) : think about this, removed const
template< typename T, typename A, typename B, typename C, typename D, typename E>
void Deserialize( T & serializer, var::variant<A, B, C, D, E> &b) {
  Relation a; // TODO: (`HUT`) : this, properly
  serializer >> a;
  b = a;
}

template< typename T>
void Serialize( T & serializer, Relation const &b) {
  serializer << b.getOp();

  // Push on value/type
  b.getValueType().match(
          [&] (int a)         { serializer << (uint32_t) 0 << a; },
          [&] (float a)       { serializer << (uint32_t) 1 << a; },
          [&] (std::string a) { serializer << (uint32_t) 2 << a; },
          [&] (bool a)        { serializer << (uint32_t) 3 << a; }
          );
}

template< typename T>
void Deserialize( T & serializer, Relation &b) {

  serializer >> b.setOp();

  uint32_t index;
  serializer >> index;
  switch(index) {
    case 0:
      {int res;
      serializer >> res;
      b.setValueType() = res;}
      break;
    case 1:
      {float res;
      serializer >> res;
      b.setValueType() = res;}
      break;
    case 2:
      {std::string res;
      serializer >> res;
      b.setValueType() = res;}
      break;
    case 3:
      {bool res;
      serializer >> res;
      b.setValueType() = res;}
      break;
    default:
      {bool res;
      serializer >> res;
      b.setValueType() = res;}
  }
}

template< typename T>
void Serialize( T & serializer, Relation::Op const &b) {

  switch(b) {
  case Relation::Op::Eq:
    serializer << (uint32_t)1;
    break;
  case Relation::Op::Lt:
    serializer << (uint32_t)2;
    break;
  case Relation::Op::Gt:
    serializer << (uint32_t)3;
    break;
  case Relation::Op::LtEq:
    serializer << (uint32_t)4;
    break;
  case Relation::Op::GtEq:
    serializer << (uint32_t)5;
    break;
  case Relation::Op::NotEq:
    serializer << (uint32_t)6;
    break;
  default:
    serializer << (uint32_t)0;
  }
}

template< typename T>
void Deserialize( T & serializer, Relation::Op &b) {
  uint32_t index;
  serializer >> index;

  switch(index) {
    case 1:
      b = Relation::Op::Eq;
      break;
    case 2:
      b = Relation::Op::Lt;
      break;
    case 3:
      b = Relation::Op::Gt;
      break;
    case 4:
      b = Relation::Op::LtEq;
      break;
    case 5:
      b = Relation::Op::GtEq;
      break;
    case 6:
      b = Relation::Op::NotEq;
      break;
    default:
      b = Relation::Op::NotEq;
  }
}
