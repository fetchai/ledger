#pragma once

//#include "serialize.h"
#include "old_oef_codebase/lib/include/serialize.h"
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

   template <typename Archive>
    explicit Attribute(const Archive &ar) : _name{ar.getString("name")},
    _type{string_to_type(ar.getString("type"))}, _required{ar.getBool("required")}
    {
      if(ar.hasMember("description"))
        _description = ar.getString("description");
    }
  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    ar.write("name", _name);
    ar.write("type", type_to_string(_type));
    ar.write("required", _required);
    if(_description)
      ar.write("description", *_description);
  }

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
  template <typename Archive, typename T>
    void serialize(Archive &ar, Type t, const T &val) const {
    std::string v{type_to_string(t)};
    ar.write("value_type", v);
    ar.write("value", val);
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
  template <typename Archive>
    explicit Relation(const Archive &ar) : _op{string_to_op(ar.getString("op"))}
    {
      std::string vt{ar.getString("value_type")};
      switch(string_to_type(vt)) {
      case Type::Int:
        _value = ar.getInt("value");
        break;
      case Type::Float:
        _value = ar.getFloat("value");
        break;
      case Type::String:
        _value = ar.getString("value");
        break;
      case Type::Bool:
        _value = ar.getBool("value");
        break;
      default:
        throw std::runtime_error(vt + std::string{" is not a valid type for Relation."});
      }
    }
  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    std::string t{"relation"};
    ar.write("type", t);
    ar.write("op", op_to_string(_op));
    _value.match([&ar,this](int s) {
        serialize<Archive,int>(ar, Type::Int, s);
      },[&ar,this](float s) {
        serialize<Archive,float>(ar, Type::Float, s);
      },[&ar,this](const std::string &s) {
        serialize<Archive,std::string>(ar, Type::String, s);
      },[&ar,this](bool s) {
        serialize<Archive,bool>(ar, Type::Bool, s);
      });
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
  template <typename Archive, typename T>
    void serialize(Archive &ar, Type t, const std::unordered_set<T> &vals) const {
    std::string v{type_to_string(t)};
    ar.write("value_type", v);
    ar.write("values", vals.begin(), vals.end());
  }
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
  template <typename Archive, typename T>
    void init_values(const Archive &ar) {
    std::unordered_set<T> res;
    ar.template getObjects<T>("values", std::inserter(res, res.end()));
    _values = res;
  }
 public:
  explicit Set(Op op, const ValueType &values) : _op{op}, _values{values} {}
  template <typename Archive>
    explicit Set(const Archive &ar) : _op{string_to_op(ar.getString("op"))}
    {
      std::string vt{ar.getString("value_type")};
      switch(string_to_type(vt)) {
      case Type::Int:
        init_values<Archive,int>(ar);
        break;
      case Type::Float:
        init_values<Archive,float>(ar);
        break;
      case Type::String:
        init_values<Archive,std::string>(ar);
        break;
      case Type::Bool:
        init_values<Archive,bool>(ar);
        break;
      default:
        throw std::runtime_error(vt + std::string{" is not a valid type for Set."});
      }
    }
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
  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    std::string t{"set"};
    ar.write("type", t);
    ar.write("op", op_to_string(_op));
    _values.match([&ar,this](const std::unordered_set<int> &s) {
        serialize<Archive,int>(ar, Type::Int, s);
      },[&ar,this](const std::unordered_set<float> &s) {
        serialize<Archive,float>(ar, Type::Float, s);
      },[&ar,this](const std::unordered_set<std::string> &s) {
        serialize<Archive,std::string>(ar, Type::String, s);
      },[&ar,this](const std::unordered_set<bool> &s) {
        serialize<Archive,bool>(ar, Type::Bool, s);
      });
  }

};
class Range {
 public:
  using ValueType = var::variant<std::pair<int,int>,std::pair<float,float>,std::pair<std::string,std::string>>;
 private:
  ValueType _pair;
  template <typename Archive, typename T>
    void serialize(Archive &ar, const std::pair<T,T> &p) const {
    std::string v{t_to_string(p.first)};
    ar.write("value_type", v);
    ar.write("start", p.first);
    ar.write("end", p.second);
  }
 public:
  explicit Range(ValueType v) : _pair{v} {}
  // enable_if is necessary to prevent both constructor to conflict.
  template <typename Archive, typename std::enable_if<!std::is_same<Archive,Range::ValueType>::value>::type* = nullptr>
    explicit Range(const Archive &ar)
    {
      std::string vt{ar.getString("value_type")};
      switch(string_to_type(vt)) {
      case Type::Int:
        _pair = std::make_pair(ar.getInt("start"), ar.getInt("end"));
        break;
      case Type::Float:
        _pair = std::make_pair(ar.getFloat("start"), ar.getFloat("end"));
        break;
      case Type::String:
        _pair = std::make_pair(ar.getString("start"), ar.getString("end"));
        break;
      default:
        throw std::runtime_error(vt + std::string{" is not a valid type for Range."});
      }
    }
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
  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    std::string t{"range"};
    ar.write("type", t);
    _pair.match([&ar,this](std::pair<int,int> p) {
        serialize<Archive,int>(ar, p);
      },[&ar,this](std::pair<float,float> p) {
        serialize<Archive,float>(ar, p);
      },[&ar,this](std::pair<std::string,std::string> p) {
        serialize<Archive,std::string>(ar, p);
      });
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

  template <typename Archive, typename std::enable_if<!std::is_same<Archive,ConstraintType::ValueType>::value>::type* = nullptr> // TODO: (`HUT`) : delete this
    explicit ConstraintType(const Archive &ar);
  template <typename Archive> void serialize(Archive &ar) const;
  //
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

  template <typename Archive> explicit Constraint(const Archive &ar);
  template <typename Archive> void serialize(Archive &ar) const;
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
  template <typename Archive>
    explicit Or(const Archive &ar) {
    std::function<void(const Archive &)> f = [this](const Archive &iar) {
      _expr.emplace_back(ConstraintType{iar});
    };
    ar.parseObjects("constraints", f);
  }
  template <typename Archive>
    void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    std::string t{"or"};
    ar.write("type", t);
    ar.write("constraints", _expr.begin(), _expr.end());
  }
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
  template <typename Archive>
    explicit And(const Archive &ar) {
    std::function<void(const Archive &)> f = [this](const Archive &iar) {
      _expr.emplace_back(ConstraintType{iar});
    };
    ar.parseObjects("constraints", f);
  }
  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    std::string t{"and"};
    ar.write("type", t);
    ar.write("constraints", _expr.begin(), _expr.end());
  }
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

template <typename Archive, typename std::enable_if<!std::is_same<Archive,ConstraintType::ValueType>::value>::type*>
  ConstraintType::ConstraintType(const Archive &ar) {
  std::string t{ar.getString("type")};
  if(t == "range") {
    _constraint = Range{ar};
  } else {
    if(t == "set") {
      _constraint = Set{ar};
    } else {
      if(t == "relation") {
        _constraint = Relation{ar};
      } else {
        if(t == "or") {
          _constraint = Or{ar};
        } else {
          if(t == "and") {
            _constraint = And{ar};
          } else {
            throw std::invalid_argument(t + " is not a valid constraint type.");
          }
        }
      }
    }
  }
}

template <typename Archive>
void ConstraintType::serialize(Archive &ar) const {
  _constraint.match([&ar,this](const Range &r) {
      ar.write(r);
    },[&ar,this](const Relation &r) {
      ar.write(r);
    },[&ar,this](const Set &r) {
      ar.write(r);
    },[&ar,this](const Or &r) {
      ar.write(r);
    },[&ar,this](const And &r) {
      ar.write(r);
    });
}

template <typename Archive>
void Constraint::serialize(Archive &ar) const {
  ObjectWrapper<Archive> o{ar};
  ar.write("attribute", _attribute);
  ar.write("constraint", _constraint);
}

template <typename Archive>
Constraint::Constraint(const Archive &ar) : _attribute{ar.getObject("attribute")}, _constraint{ar.getObject("constraint")}  {}

class KeywordLookup
{
 public:
  explicit KeywordLookup(std::vector<std::string> keywords) :
    _keywords(keywords) {}

  template <typename Archive>
  explicit KeywordLookup(const Archive &ar) { _keywords.push_back(std::string{"asfdsf"}); std::cout << "happening" << std::endl; }

  //template <typename Archive>
  //explicit QueryModel(const Archive &ar)
  //{
  //  std::function<void(const Archive &)> f = [this](const Archive &iar) {
  //    _constraints.emplace_back(Constraint{iar});
  //  };
  //  ar.parseObjects("constraints", f);
  //  if(ar.hasMember("schema"))
  //    _model = DataModel{ar.getObject("schema")};
  //}

  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    ar.write("keywords", _keywords.begin(), _keywords.end());
  }

  std::vector<std::string> getKeywords() const { return _keywords; }

  //template <typename T>
  //  bool check_value(const T &v) const {
  //  for(auto &c : _constraints) {
  //    if(!c.check(VariantType{v}))
  //      return false;
  //  }
  //  return true;
  //}
  //bool check(const Instance &i) const {
  //  if(_model) {
  //    if(_model->name() != i.model().name())
  //      return false;
  //    // TODO: more to compare ?
  //  }
  //  for(auto &c : _constraints) {
  //    if(!c.check(i))
  //      return false;
  //  }
  //  return true;
  //}

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
  template <typename Archive>
    explicit SchemaRef(const Archive &ar) : _name{ar.getString("name")}, _version{ar.getUint("version")} {}
  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    ar.write("name", _name);
    ar.write("version", _version);
  }
  std::string name() const { return _name; }
  uint32_t version() const { return _version; }
};

class Schema {
 private:
  uint32_t _version;
  DataModel _schema;
 public:
  explicit Schema(uint32_t version, const DataModel &schema) : _version{version}, _schema{schema} {}
  template <typename Archive>
    explicit Schema(const Archive &ar) : _version{ar.getUint("version")}, _schema{ar.getObject("schema")}  {}
  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    ar.write("version", _version);
    ar.write("schema", _schema);
  }
  uint32_t version() const { return _version; }
  DataModel schema() const { return _schema; }
};

class Schemas {
 private:
  mutable std::mutex _lock;
  std::vector<Schema> _schemas;
 public:
  explicit Schemas() = default;
  template <typename Archive>
  void serialize(Archive &ar) const {
    ar.write(_schemas.begin(), _schemas.end());
  }
  template <typename Archive>
    explicit Schemas(const Archive &ar) {
    std::function<void(const Archive &)> f = [this](const Archive &iar) {
      _schemas.emplace_back(Schema{iar});
    };
    ar.parseObjects(f);
  }
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
  template <typename Archive>
    explicit SchemaDirectory(const Archive &ar) {
    std::function<void(const Archive &)> f = [this](const Archive &iar) {
      _schemas.emplace(std::make_pair(iar.getString("name"),
                                      Schemas{iar.getObject("schemas")}));
    };
    ar.parseObjects("schemaDirectory", f);
  }
  template <typename Archive>
  void serialize(Archive &ar) const {
    ObjectWrapper<Archive> o{ar};
    ar.write("schemaDirectory", _schemas, "name", "schemas");
  }
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

// all ser/deser methods

// TODO: (`HUT`) : ask troels why this doesn't work - can't find 
template< typename T, typename N, typename M>
void Serialize( T & serializer, const std::unordered_map<std::string, std::string> &b) {
  for(auto i : b){
    serializer << i.first << i.second;
  }
}

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
          [&] (A a)    { std::cerr << "found " << "2" << std::endl; },
          [&] (B a)    { std::cerr << "found " << "3" << std::endl; },
          [&] (C a)    { std::cerr << "found " << "4" << std::endl; },
          [&] (Relation a)    { serializer << a; }, // TODO: (`HUT`) : the rest of them
          [&] (E a)    { std::cerr << "found " << "6" << std::endl; }
          );
}

// TODO: (`HUT`) : think about this, removed const
template< typename T, typename A, typename B, typename C, typename D, typename E>
void Deserialize( T & serializer, var::variant<A, B, C, D, E> &b) {
  Relation a; // TODO: (`HUT`) : this properly
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
  //serializer >> b.setValueType();

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
  //serializer >> b.getConstraint();
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

//// ValueType ser/deser - no work, why? 
//template< typename T>
//void Deserialize( T & serializer, var::variant<int,float,std::string,bool> &b) {
//}
