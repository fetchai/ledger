#ifndef SCHEMA_DEFINES_H
#define SCHEMA_DEFINES_H

#include<mutex>
#include<unordered_set>
#include<unordered_map>

// TODO: (`HUT`) : probably remove with time
#include<iostream>
#include<experimental/optional>
#include<mapbox/variant.hpp>

#include"json/document.hpp"

// Schema for the OEF: effectively defining the class structure that builds DataModels, Instances and Queries for those Instances

namespace stde = std::experimental;
namespace var = mapbox::util; // for the variant

enum class Type { Float, Int, Bool, String };
using VariantType = var::variant<int,float,std::string,bool>;
VariantType string_to_value(Type t, const std::string &s);

// TODO: (`HUT`) : put this class conversion stuff into another file, perhaps
Type string_to_type(const std::string &s) {
  if(s == "float")
    return Type::Float;
  if(s == "int")
    return Type::Int;
  if(s == "bool")
    return Type::Bool;
  if(s == "string")
    return Type::String;
  throw std::invalid_argument(s + std::string(" is not a valid type"));
}

class Attribute {
private:
  std::string _name; // TODO: (`HUT`) : rename all these to type_
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

  // Getters and setters for serialization
  const std::string &getName() const     { return _name; }
  std::string       &setName()           { return _name; }
  const bool        &getRequired() const { return _required; }
  bool              &setRequired()       { return _required; }
  const Type        &getType() const     { return _type; }
  Type              &setType()           { return _type; }

  Attribute(fetch::json::JSONDocument &jsonDoc) {
    _name     = std::string(jsonDoc["name"].as_byte_array());
    _type     = string_to_type(std::string(jsonDoc["type"].as_byte_array()));
    _required = jsonDoc["required"].as_bool();
  }
};

class Relation {
public:
  Relation() {}
  using ValueType = var::variant<int,float,std::string,bool>;
  enum class Op { Eq, Lt, Gt, LtEq, GtEq, NotEq };
private:
  Op _op;
  ValueType _value;
public:
  static std::string op_to_string(Op op) {
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
  static Op string_to_op(const std::string &s) {
    if(s == "=") return Op::Eq;
    if(s == "<") return Op::Lt;
    if(s == "<=") return Op::LtEq;
    if(s == ">") return Op::Gt;
    if(s == ">=") return Op::GtEq;
    if(s == "<>") return Op::NotEq;
    throw std::invalid_argument(s + std::string{" is not a valid operator."});
  }
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

  // Getters and setters for serialization
  const Op        &getOp() const        { return _op; }
  Op              &setOp()              { return _op; }
  const ValueType &getValueType() const { return _value; }
  ValueType       &setValueType()       { return _value; }
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

  // Getters and setters for serialization
  const Op        &getOp() const         { return _op; }
  Op              &setOp()               { return _op; }
  const ValueType &getValueTypes() const { return _values; }
  ValueType       &setValueTypes()       { return _values; }
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

  // Getters and setters for serialization
  //std::string              name() const { return _name; } // TODO: (`HUT`) : old, delete when appropriate
  //std::vector<std::string> keywords() const { return _keywords; }
  const std::string              &getName() const       { return _name; }
  std::string                    &setName()             { return _name; }
  const std::vector<std::string> &getKeywords() const   { return _keywords; }
  std::vector<std::string>       &setKeywords()         { return _keywords; }
  const std::vector<Attribute>   &getAttributes() const { return _attributes; }
  std::vector<Attribute>         &setAttributes()       { return _attributes; }

  // JSON construction
  template<typename T>
  DataModel(T &jsonDoc)
  {
    LOG_STACK_TRACE_POINT;

    _name = std::string(jsonDoc["name"].as_byte_array());

    for(auto &a: jsonDoc["attributes"].as_array()) {

      fetch::json::JSONDocument doc; // TODO: (`HUT`) : ask Troells if is correct use
      doc.root() = a;

      Attribute attribute(doc);
      _attributes.push_back(attribute);
    }
  }
};

class Instance {
private:
  DataModel _model;
  std::unordered_map<std::string,std::string> _values;

public:
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
    std::size_t h = std::hash<std::string>{}(_model.getName());
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

  //template <typename T>
  Instance(fetch::json::JSONDocument &jsonDoc) // TODO: (`HUT`) : make generic, also not sure non-const is correct, ask troells
    : _model{jsonDoc["schema"]}
  {
    LOG_STACK_TRACE_POINT; // TODO: (`HUT`) : put this everywhere

    for(auto &a: jsonDoc["values"].as_array()) {
      for(auto &b: a.as_dictionary()) {
        std::string first(b.first);
        std::string second(b.second.as_byte_array());
        _values[first] = second;
      }
    }
  }

  // Getters and setters for serialization
  const std::unordered_map<std::string,std::string> &getValues() const    { return _values; }
  std::unordered_map<std::string,std::string>       &setValues()          { return _values; }
  const DataModel                                   &getDataModel() const { return _model; }
  DataModel                                         &setDataModel()       { return _model; }
};

// Used for hashing classes for use in unordered_map etc.
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

  // Getters and setters for serialization
  const ConstraintType::ValueType& getConstraint() const { return _constraint; }
  ConstraintType::ValueType& setConstraint()             { return _constraint; }

  ConstraintType(fetch::json::JSONDocument &jsonDoc)
  {
    LOG_STACK_TRACE_POINT;

    // TODO: (`HUT`) : all possible types
    std::string type = jsonDoc["type"].as_byte_array();

    if(type.compare("relation") == 0) {
      _constraint = Relation(Relation::string_to_op(jsonDoc["op"].as_byte_array()), jsonDoc["value"].as_bool());
    }
  }
};
                    //"type": "relation",
                    //"op": "=",
                    //"value_type": "bool",
                    //"value": True

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
    VariantType value{string_to_value(_attribute.type(), *v)};
    return check(value);
  }

  // Getters and setters for serialization
  const Attribute      &getAttribute() const      { return _attribute; }
  Attribute            &setAttribute()            { return _attribute; }
  const ConstraintType &getConstraintType() const { return _constraint; }
  ConstraintType       &setConstraintType()       { return _constraint; }

  Constraint(fetch::json::JSONDocument &jsonDoc)
  {
    LOG_STACK_TRACE_POINT;

    fetch::json::JSONDocument doc;
    doc.root() = jsonDoc["attribute"];

    Attribute attribute(doc);
    _attribute = attribute;

    doc.root() = jsonDoc["constraint"];
    ConstraintType constraintType(doc);
    _constraint = constraintType;
  }
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

  // Getters and setters for serialization
  const std::vector<ConstraintType> &getExpressions() const { return _expr; }
  std::vector<ConstraintType>       &setExpressions()       { return _expr; }
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
  stde::optional<DataModel> _model; // TODO: (`HUT`) : this is not serialized yet, nor JSON-ed
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
      if(_model->getName() != i.model().getName())
        return false;
      // TODO: more to compare ?
    }
    for(auto &c : _constraints) {
      if(!c.check(i))
        return false;
    }
    return true;
  }

  // Getters and setters for serialization
  const std::vector<Constraint> &getConstraints() const { return _constraints; }
  std::vector<Constraint>       &setConstraints()       { return _constraints; }

  QueryModel(fetch::json::JSONDocument &jsonDoc) {
    LOG_STACK_TRACE_POINT;

    for(auto &a: jsonDoc["constraints"].as_array()) {

      fetch::json::JSONDocument doc; // TODO: (`HUT`) : ask Troells if is correct use
      doc.root() = a;

      Constraint constraint(doc);
      _constraints.push_back(constraint);
    }
  }
};

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

/////////////////////////////////////////////////////////////////////////////////////
// Functionality unused in current code below

/*
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
*/

#endif
