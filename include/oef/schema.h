#pragma once

//#include "serialize.h"
//#include "old_oef_codebase/lib/include/serialize.h"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <limits>
#include <unordered_set>
#include <experimental/optional>
#include "mapbox/variant.hpp"

namespace stde = std::experimental;
namespace var = mapbox::util; // for the variant

enum class Type { Float, Int, Bool, String };
using VariantType = var::variant<int,float,std::string,bool>;
std::string type_to_string(Type t);
Type string_to_type(const std::string &s);
VariantType string_to_value(Type t, const std::string &s);

class Attribute {
 private:
  std::string _name;
  Type _type;
  bool _required;
  stde::optional<std::string> _description;
  bool validate(const std::string &value) const {
    try {
      switch(_type) {
      case Type::Float:
        (void)std::stod(value);
        break;
      case Type::Int:
        (void)std::stol(value);
        break;
      case Type::Bool:
        return value == "true" || value == "false" || value == "1" || value == "0";
        break;
      case Type::String:
        return true;
        break;
      }
    } catch(std::exception &e) {
      return false;
    }
    return true;
  }
 public:
  explicit Attribute() {}
  explicit Attribute(const std::string &name, Type type, bool required, stde::optional<std::string> description = stde::nullopt)
    : _name{name}, _type{type}, _required{required}, _description{description} {}

  std::string name() const { return _name; }
  Type type() const { return _type; }
  bool required() const { return _required; }

  std::pair<std::string,std::string> instantiate(const std::unordered_map<std::string,std::string> &values) const {
    auto iter = values.find(_name);
    if(iter == values.end()) {
      if(_required) {
        throw std::invalid_argument(std::string("Missing value: ") + _name);
      }
      return std::make_pair(_name,"");
    }
    if(validate(iter->second)) {
      return std::make_pair(_name, iter->second);
    }
    throw std::invalid_argument(_name + std::string(" has a wrong type of value ") + iter->second);
  }

  // getters/setters

  const std::string& getName() const { return _name; } // TODO: (`HUT`) : make this auto?
  std::string& setName()             { return _name; }

  const bool& getRequired() const { return _required; }
  bool& setRequired()             { return _required; }

  const Type& getType() const { return _type; } // TODO: (`HUT`) : probably not so great for troells serialization
  Type& setType()             { return _type; }
};

std::string t_to_string(int i);
std::string t_to_string(float f);
std::string t_to_string(bool b);
std::string t_to_string(const std::string &s);

class Relation {
 public:
  Relation() {}
  using ValueType = var::variant<int,float,std::string,bool>;
  enum class Op { Eq, Lt, Gt, LtEq, GtEq, NotEq };
 private:
  Op _op;
  ValueType _value;
  std::string op_to_string(Op op) const {
    switch(op) {
    case Op::Eq: return "=";
    case Op::Lt: return "<";
    case Op::LtEq: return "<=";
    case Op::Gt: return ">";
    case Op::GtEq: return ">=";
    case Op::NotEq: return "<>";
    }
    return "";
  }
  Op string_to_op(const std::string &s) const {
    if(s == "=") return Op::Eq;
    if(s == "<") return Op::Lt;
    if(s == "<=") return Op::LtEq;
    if(s == ">") return Op::Gt;
    if(s == ">=") return Op::GtEq;
    if(s == "<>") return Op::NotEq;
    throw std::invalid_argument(s + std::string{" is not a valid operator."});
  }
 public:
  explicit Relation(Op op, const ValueType &value) : _op{op}, _value{value} {}
  template <typename T>
    bool check_value(const T &v) const {
    auto &s = _value.get<T>();
    switch(_op) {
    case Op::Eq: return s == v;
    case Op::NotEq: return s != v;
    case Op::Lt: return v < s;
    case Op::LtEq: return v <= s;
    case Op::Gt: return v > s;
    case Op::GtEq: return v >= s;
    }
    return false;
  }
  bool check(const VariantType &v) const {
    bool res = false;
    v.match([&res,this](int i) {
        res = check_value(i);
      },[&res,this](float f) {
        res = check_value(f);
      },[&res,this](const std::string &s) {
        res = check_value(s);
      },[&res,this](bool b) {
        res = check_value(b);
      });
    return res;
  }

  // Getters and setters for relation

  const Op& getOp() const               { return _op; }
  Op& setOp()                           { return _op; }
  const ValueType& getValueType() const { return _value; }
  ValueType& setValueType()             { return _value; }
};

class Set {
 public:
  using ValueType = var::variant<std::unordered_set<int>,std::unordered_set<float>,
    std::unordered_set<std::string>,std::unordered_set<bool>>;
  enum class Op { In, NotIn };
 private:
  Op _op;
  ValueType _values;
  std::string op_to_string(Op op) const {
    switch(op) {
    case Op::In: return "in";
    case Op::NotIn: return "not in";
    }
    return "";
  }
  Op string_to_op(const std::string &s) const {
    if(s == "in") return Op::In;
    if(s == "not in") return Op::NotIn;
    throw std::invalid_argument(s + std::string{" is not a valid operator."});
  }
 public:
  explicit Set(Op op, const ValueType &values) : _op{op}, _values{values} {}
  bool check(const VariantType &v) const {
    bool res = false;
    v.match([&res,this](int i) {
        auto &s = _values.get<std::unordered_set<int>>();
        res = s.find(i) != s.end();
      },[&res,this](float f) {
        auto &s = _values.get<std::unordered_set<float>>();
        res = s.find(f) != s.end();
      },[&res,this](const std::string &st) {
        auto &s = _values.get<std::unordered_set<std::string>>();
        res = s.find(st) != s.end();
      },[&res,this](bool b) {
        auto &s = _values.get<std::unordered_set<bool>>();
        res = s.find(b) != s.end();
      });
    return res;
  }
};
class Range {
 public:
  using ValueType = var::variant<std::pair<int,int>,std::pair<float,float>,std::pair<std::string,std::string>>;
 private:
  ValueType _pair;
 public:
  explicit Range(ValueType v) : _pair{v} {}
  // enable_if is necessary to prevent both constructor to conflict.
  //
  bool check(const VariantType &v) const {
    bool res = false;
    v.match([&res,this](int i) {
        auto &p = _pair.get<std::pair<int,int>>();
        res = i >= p.first && i <= p.second;
      },[&res,this](float f) {
        auto &p = _pair.get<std::pair<float,float>>();
        res = f >= p.first && f <= p.second;
      },[&res,this](const std::string &s) {
        auto &p = _pair.get<std::pair<std::string,std::string>>();
        res = s >= p.first && s <= p.second;
      },[&res,this](bool b) {
        res = false; // doesn't make sense
      });
    return res;
  }
};

class DataModel {
 private:
  std::string                 _name;
  std::vector<Attribute>      _attributes;
  std::vector<std::string>    _keywords;
  //stde::optional<std::string> _description;

 public:
  explicit DataModel() {}

  explicit DataModel(const std::string &name, const std::vector<Attribute> &attributes) :
    _name{name},
    _attributes{attributes} {}
  //, _description{description} {}

  // nhutton: Temporary keyword testing
  void addKeywords(std::vector<std::string> keywords)
  {
    for(auto i : keywords)
    {
      _keywords.push_back(i);
    }
  }

  bool operator==(const DataModel &other) const
  {
    if(_name != other._name)
      return false;
    // TODO: should check more.
    return true;
  }

  stde::optional<Attribute> attribute(const std::string &name) const
  {
    for(auto &a : _attributes) {
      if(a.name() == name)
        return stde::optional<Attribute>{a};
    }
    return stde::nullopt;
  }

  std::vector<std::pair<std::string,std::string>> instantiate(const std::unordered_map<std::string,std::string> &values) const
  {
    std::vector<std::pair<std::string,std::string>> res;
    for(auto &a : _attributes)
    {
      res.emplace_back(a.instantiate(values));
    }
    return res;
  }

  // getters/setters 
  std::string              name() const { return _name; } // TODO: (`HUT`) : old, delete when appropriate
  std::vector<std::string> keywords() const { return _keywords; }

  const std::string& getName() const { return _name; } // TODO: (`HUT`) : make this auto?
  std::string& setName()             { return _name; }

  const std::vector<std::string>& getKeywords() const { return _keywords; } // TODO: (`HUT`) : make this auto?
  std::vector<std::string>& setKeywords()             { return _keywords; }

  const std::vector<Attribute>& getAttributes() const { return _attributes; } // TODO: (`HUT`) : make this auto?
  std::vector<Attribute>& setAttributes()             { return _attributes; }
};

class Instance {
 private:
  DataModel _model;
  std::unordered_map<std::string,std::string> _values;

 public:
  // getters/setters for serialization

  const std::unordered_map<std::string,std::string>& getValues() const { return _values; }
  std::unordered_map<std::string,std::string>& setValues()             { return _values; }

  const DataModel& getDataModel() const { return _model; }
  DataModel& setDataModel()             { return _model; }

  Instance() {}

  Instance(const DataModel &model, const std::unordered_map<std::string,std::string> &values)
    : _model{model}, _values{values} {}

  bool operator==(const Instance &other) const
  {
    if(!(_model == other._model))
      return false;
    for(const auto &p : _values) {
      const auto &iter = other._values.find(p.first);
      if(iter == other._values.end())
        return false;
      if(iter->second != p.second)
        return false;
    }
    return true;
  }
  std::size_t hash() const {
    std::size_t h = std::hash<std::string>{}(_model.name());
    for(const auto &p : _values) {
      std::size_t hs = std::hash<std::string>{}(p.first);
      h = hs ^ (h << 1);
      hs = std::hash<std::string>{}(p.second);
      h = hs ^ (h << 2);
    }
    return h;
  }

  std::vector<std::pair<std::string,std::string>>
    instantiate() const {
    return _model.instantiate(_values);
  }
  DataModel model() const {
    return _model;
  }
  stde::optional<std::string> value(const std::string &name) const {
    auto iter = _values.find(name);
    if(iter == _values.end())
      return stde::nullopt;
    return stde::optional<std::string>{iter->second};
  }
};

namespace std
{
  template<> struct hash<Instance>  {
    typedef Instance argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept
    {
      return s.hash();
    }
  };
}

class Or;
class And;

class ConstraintType {
 public:
  using ValueType = var::variant<var::recursive_wrapper<Or>,var::recursive_wrapper<And>,Range,Relation,Set>;
 private:
  ConstraintType::ValueType _constraint;
 public:
  explicit ConstraintType() {}

  explicit ConstraintType(ValueType v) : _constraint{v} {}

  bool check(const VariantType &v) const;

  // getters/setters
  const ConstraintType::ValueType& getConstraint() const { return _constraint; }
  ConstraintType::ValueType& setConstraint()             { return _constraint; }
};

class Constraint {
 private:
  Attribute _attribute;
  ConstraintType _constraint;
 public:
  explicit Constraint() {}

  explicit Constraint(const Attribute &attribute, const ConstraintType &constraint)
    : _attribute{attribute}, _constraint{constraint} {}

  bool check(const VariantType &v) const {
    return _constraint.check(v);
  }
  bool check(const Instance &i) const {
    const auto &model = i.model();
    auto attr = model.attribute(_attribute.name());
    if(attr) {
      if(attr->type() != _attribute.type())
        return false;
    }
    auto v = i.value(_attribute.name());
    if(!v) {
      if(_attribute.required())
        std::cerr << "Should not happen!\n"; // Exception ?
      return false;
    }
    VariantType value{string_to_value(_attribute.type(), *v)};// this is a prblem
    return check(value);
  }

  // getters/setters
  const Attribute& getAttribute() const { return _attribute; }
  Attribute& setAttribute()             { return _attribute; }

  const ConstraintType& getConstraintType() const { return _constraint; }
  ConstraintType& setConstraintType()             { return _constraint; }
};

class Or {
  std::vector<ConstraintType> _expr;
 public:
  explicit Or(const std::vector<ConstraintType> expr) : _expr{expr} {}
  explicit Or() {}; // to make happy variant.

  bool check(const VariantType &v) const {
    for(auto &c : _expr) {
      if(c.check(v))
        return true;
    }
    return false;
  }
};

class And {
 private:
  std::vector<ConstraintType> _expr;
 public:
  explicit And(const std::vector<ConstraintType> expr) : _expr{expr} {}

  bool check(const VariantType &v) const {
    for(auto &c : _expr) {
      if(!c.check(v))
        return false;
    }
    return true;
  }

  // Getters and setters
  const std::vector<ConstraintType>& getExpressions() const { return _expr; }
  std::vector<ConstraintType>& setExpressions()             { return _expr; }
};

class KeywordLookup
{
 public:
  explicit KeywordLookup(std::vector<std::string> keywords) :
    _keywords(keywords) {}

  std::vector<std::string> getKeywords() const { return _keywords; }

 private:
  std::vector<std::string> _keywords;
};

class QueryModel {
 private:
  std::vector<Constraint> _constraints;
  stde::optional<DataModel> _model; // TODO: (`HUT`) : this is not serialized yet
 public:

  explicit QueryModel() : _model{stde::nullopt} {}

  explicit QueryModel(const std::vector<Constraint> &constraints, stde::optional<DataModel> model = stde::nullopt)
    : _constraints{constraints}, _model{model} {}

  template <typename T>
    bool check_value(const T &v) const {
    for(auto &c : _constraints) {
      if(!c.check(VariantType{v}))
        return false;
    }
    return true;
  }
  bool check(const Instance &i) const {
    if(_model) {
      if(_model->name() != i.model().name())
        return false;
      // TODO: more to compare ?
    }
    for(auto &c : _constraints) {
      if(!c.check(i))
        return false;
    }
    return true;
  }

  // getters/setters for serialization
  const std::vector<Constraint>& getConstraints() const { return _constraints; }
  std::vector<Constraint>& setConstraints()             { return _constraints; }
};

class SchemaRef {
 private:
  std::string _name; // unique
  uint32_t _version;
 public:
  explicit SchemaRef(const std::string &name, uint32_t version) : _name{name}, _version{version} {}

  std::string name() const { return _name; }
  uint32_t version() const { return _version; }
};

class Schema {
 private:
  uint32_t _version;
  DataModel _schema;
 public:
  explicit Schema(uint32_t version, const DataModel &schema) : _version{version}, _schema{schema} {}
  uint32_t version() const { return _version; }
  DataModel schema() const { return _schema; }
};

class Schemas {
 private:
  mutable std::mutex _lock;
  std::vector<Schema> _schemas;
 public:
  explicit Schemas() = default;
  uint32_t add(uint32_t version, const DataModel &schema) {
    std::lock_guard<std::mutex> lock(_lock);
    if(version == std::numeric_limits<uint32_t>::max())
      version = _schemas.size() + 1;
    _schemas.emplace_back(Schema(version, schema));
    return version;
  }
  stde::optional<Schema> get(uint32_t version) const {
    std::lock_guard<std::mutex> lock(_lock);
    if(_schemas.size() == 0)
      return stde::nullopt;
    if(version == std::numeric_limits<uint32_t>::max())
      return _schemas.back();
    for(auto &p : _schemas) { // TODO: binary search
      if(p.version() >= version)
        return p;
    }
    return _schemas.back();
  }
};

class SchemaDirectory {
 private:
  std::unordered_map<std::string, Schemas> _schemas;
 public:
  explicit SchemaDirectory() = default;

  stde::optional<Schema> get(const std::string &key, uint32_t version = std::numeric_limits<uint32_t>::max()) const {
    const auto &iter = _schemas.find(key);
    if(iter != _schemas.end())
      return iter->second.get(version);
    return stde::nullopt;
  }
  uint32_t add(const std::string &key, const DataModel &schema, uint32_t version = std::numeric_limits<uint32_t>::max()) {
    return _schemas[key].add(version, schema);
  }
};


//// TODO: (`HUT`) : maybe ask troels why this doesn't work - can't find template
//template< typename T, typename N, typename M>
//void Serialize( T & serializer, const std::unordered_map<std::string, std::string> &b) {
//  for(auto i : b){
//    serializer << i.first << i.second;
//  }
//}

// Temporarily place convenience fns here
bool ConstraintType::check(const VariantType &v) const {
  bool res = false;
  _constraint.match([&v,&res](const Range &r) {
      res = r.check(v);
    },[&v,&res](const Relation &r) {
      res =  r.check(v);
    },[&v,&res](const Set &r) {
      res = r.check(v);
    },[&v,&res](const Or &r) {
      res = r.check(v);
    },[&v,&res](const And &r) {
      res = r.check(v);
    });
  return res;
}

VariantType string_to_value(Type t, const std::string &s) {
  switch(t) {
  case Type::Float:
    return VariantType{float(std::stod(s))};
  case Type::Int:
    return VariantType{int(std::stol(s))};
  case Type::String:
    return VariantType{s};
  case Type::Bool:
    return VariantType{s == "1" || s == "true"};
  }
  // should not reach this line
  return VariantType{std::string{""}};
}

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
