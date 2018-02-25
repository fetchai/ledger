// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// File to enable JSON strings to be packed and unpacked on both ends of a communication link

#pragma once

#include <stdexcept>

class AssertException : public std::logic_error {
public:
  AssertException(const char* w) : std::logic_error(w) {}
  AssertException(const AssertException& rhs) : std::logic_error(rhs) {}
};

#define RAPIDJSON_ASSERT(x) if (!(x)) throw AssertException(RAPIDJSON_STRINGIFY(x))

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stream.h"
#include <type_traits>
#include <fstream>
#include <iostream>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

// This class will create a JSON archive
template <typename Writer>
class JSONOutputArchive {

public:

  JSONOutputArchive(Writer &writer) : _writer{writer} {}

  void startObject() { _writer.StartObject(); }
  void endObject()   { _writer.EndObject(); }

  template <typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void write(const std::string &key, T val) {
    addKey(key);
    write(val);
  }
  void write(const std::string &key, const std::string &val) {
    addKey(key);
    _writer.String(val.c_str(), static_cast<rapidjson::SizeType>(val.length()));
  }
  template <typename T>
  void write(const std::string &key, const std::vector<T*> &vals) {
    addKey(key);
    _writer.StartArray();
    for(auto *v : vals) {
      v->serialize(*this);
    }
    _writer.EndArray();
  }
  template <typename T>
    void write(const T &val) {
    val.serialize(*this);
  }
  template <typename D, typename T>
    void write(const std::string &key, const std::vector<std::pair<D, T>> &vals, const std::string &name) {
    addKey(key);
    _writer.StartArray();
    for(auto &v : vals) {
      startObject();
      write(name, v.first);
      endObject();
    }
    _writer.EndArray();
  }
  template <typename K, typename V>
    void write(const std::string &key, const std::unordered_map<K,V> &vals, const std::string &key1, const std::string &key2) {
    addKey(key);
    _writer.StartArray();
    for(auto &v : vals) {
      startObject();
      write(key1, v.first);
      write(key2, v.second);
      endObject();
    }
    _writer.EndArray();
  }
  template <typename V>
    void write(const std::string &key, const std::unordered_map<std::string,V> &vals) {
    addKey(key);
    _writer.StartArray();
    for(auto &v : vals) {
      startObject();
      write(v.first, v.second);
      endObject();
    }
    _writer.EndArray();
  }
  template <typename Iter>
  void write(const std::string &key, Iter begin, Iter end) {
    addKey(key);
    _writer.StartArray();
    while(begin != end) {
      write(*begin);
      ++begin;
    }
    _writer.EndArray();
  }  
  template <typename Iter>
  void write(Iter begin, Iter end) {
    _writer.StartArray();
    while(begin != end) {
      write(*begin);
      ++begin;
    }
    _writer.EndArray();
  }  
  template <typename T>
  void write(const std::string &key, const std::vector<T*> &vals, const std::function<void(T *)>&task) {
    addKey(key);
    _writer.StartArray();
    for(auto *v : vals) {
      task(v);
    }
    _writer.EndArray();
  }
  template <typename T, typename std::enable_if<!std::is_arithmetic<T>::value>::type* = nullptr>
  void write(const std::string &key, const T &t) {
    addKey(key);
    t.serialize(*this);
  }

private:

  Writer &_writer;

  void addKey(const std::string &key) {
    _writer.Key(key.c_str(), static_cast<rapidjson::SizeType>(key.length()));
  }

  void write(bool val)               { _writer.Bool(val); }
  void write(int val)                { _writer.Int(val); }
  void write(uint32_t val)           { _writer.Uint(val); }
  void write(uint64_t val)           { _writer.Uint64(val); }
  void write(double val)             { _writer.Double(val); }
  void write(float val)              { _writer.Double(val); }
  void write(const std::string &val) { _writer.String(val.c_str(), static_cast<rapidjson::SizeType>(val.length())); }
};

class JSONInputArchive {
private:
  std::shared_ptr<rapidjson::Document> _d{new rapidjson::Document()};
  mutable rapidjson::Value _value;
  JSONInputArchive(const JSONInputArchive &ar, const char *key) : _d{ar._d} {
    auto &v{ar._value[key]};
    _value.Swap(v);
  }
 JSONInputArchive(const JSONInputArchive &ar, rapidjson::Value &array, rapidjson::SizeType index) : _d{ar._d} {
    auto &v{array[index]};
    _value.Swap(v);
  }
  void get(const rapidjson::Value &val, bool &res) const { res = val.GetBool(); }
  void get(const rapidjson::Value &val, uint32_t &res) const { res = val.GetUint(); }
  void get(const rapidjson::Value &val, int &res) const { res = val.GetInt(); }
  void get(const rapidjson::Value &val, float &res) const { res = val.GetFloat(); }
  void get(const rapidjson::Value &val, std::string &res) const { res = val.GetString(); }
public:
  template <typename S>
  JSONInputArchive(S &stream) {
    _d->ParseStream(stream);
    _value = _d->GetObject();
  }
  bool getBool(const char *key) const { return _value[key].GetBool(); }
  int getInt(const char *key) const { return _value[key].GetInt(); }
  uint32_t getUint(const char *key) const { return _value[key].GetUint(); }
  uint64_t getUint64(const char *key) const { return _value[key].GetUint64(); }
  double getDouble(const char *key) const { return _value[key].GetDouble(); }
  float getFloat(const char *key) const { return float(_value[key].GetDouble()); }
  std::string getString(const char *key) const { return _value[key].GetString(); }
  std::string getString() const { return _value.GetString(); }
  bool hasMember(const char *key) const { return _value.HasMember(key); }

  template <typename T>
  std::vector<T *> getObjects(const char *key) const {
    std::vector<T *> res;
    rapidjson::Value &array = _value[key];
    for(rapidjson::SizeType i = 0; i < array.Size(); ++i) { 
      res.push_back(new T{JSONInputArchive(*this, array, i)}); 
    } 
    return res;
  }

  std::vector<std::string> getObjectsVector(const char *key) const {
    std::vector<std::string> res;

    //res.push_back("hullabaloo");
    //res.push_back("thisisok");

    for(auto iter = _value.MemberBegin(); iter != _value.MemberEnd(); ++iter) {
      res.push_back(iter->name.GetString());
    }

    //rapidjson::Value &array = _value[key];
    //for(rapidjson::SizeType i = 0; i < array.Size(); ++i) {
    //  res.push_back(JSONInputArchive(*this, array, i).getString());
    //}

    return res;
  }
  template <typename T, typename OutputIter>
  void getObjects(const char *key, OutputIter out) const {
    rapidjson::Value &array = _value[key];
    for(rapidjson::SizeType i = 0; i < array.Size(); ++i) {
      T t;
      get(array[i], t);
      *out++ = t;
    } 
  }
  void parseObjects(const char *key, const std::function<void(const JSONInputArchive &ar)>&task) const {
    rapidjson::Value &array = _value[key];
    for(rapidjson::SizeType i = 0; i < array.Size(); ++i) {
      JSONInputArchive ar{*this, array, i};
      task(ar);
    } 
  }
  void parseObjects(const std::function<void(const JSONInputArchive &ar)>&task) const {
    rapidjson::Value &array = _value;
    for(rapidjson::SizeType i = 0; i < array.Size(); ++i) {
      JSONInputArchive ar{*this, array, i};
      task(ar);
    } 
  }
  JSONInputArchive getObject(const char *key) const { return JSONInputArchive(*this, key); }
  std::vector<std::string> getFieldsName() const {
    std::vector<std::string> res;
    for(auto iter = _value.MemberBegin(); iter != _value.MemberEnd(); ++iter) {
      res.push_back(iter->name.GetString());
    }
    return res;
  }
};

template <typename T>
class ObjectWrapper {
public:
  T &ar;
  ObjectWrapper(T &a) : ar{a} {
    ar.startObject();
  }
  ~ObjectWrapper() {
    ar.endObject();
  }
};



// Below functions do not appear to be used in codebase
// Looks like they are for packing and unpacking binary instead of JSON

template <typename OutputStream>
class BinaryOutputArchive {
 private:
  OutputStream _stream;
 public:
  BinaryOutputArchive(OutputStream &stream) {
    _stream.swap(stream);
  }
  void startObject() { }
  void endObject() { }
  template<class T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
  void write(const std::string &, const T &val) {
    _stream.write(reinterpret_cast<const char *>(&val), sizeof(val));
  }
  void write(const std::string &, const std::string &val) {
    size_t len = val.length();
    _stream.write(reinterpret_cast<const char *>(&len), sizeof(len));
    _stream.write(val.c_str(), len);
  }
  template <typename T>
  void write(const std::string &, const std::vector<T*> &vals) {
    size_t len = vals.size();
    _stream.write(reinterpret_cast<const char *>(&len), sizeof(len));
    for(auto *v : vals) {
      v->serialize(*this);
    }
  }
  template <typename Iter>
  void write(const std::string &, Iter begin, Iter end) {
    size_t len = 0;
    Iter begin2 = begin;
    while(begin2 != end) {
      ++len;
      ++begin2;
    }
    _stream.write(reinterpret_cast<const char *>(&len), sizeof(len));
    while(begin != end) {
      auto val = *begin;
      _stream.write(reinterpret_cast<const char *>(&val), sizeof(val));
      ++begin;
    }
  }
  template <typename T>
  void write(const std::string &, const std::vector<T*> &vals, const std::function<void(T *)>&task) {
    size_t len = vals.size();
    _stream.write(reinterpret_cast<const char *>(&len), sizeof(len));
    for(auto *v : vals) {
      task(v);
    }
  }
  template <typename T, typename std::enable_if<!std::is_arithmetic<T>::value>::type* = nullptr>
  void write(const std::string &, const T &t) {
    t.serialize(*this);
  }
};

template <typename InputStream>
class BinaryInputArchive {
 private:
  std::shared_ptr<InputStream> _stream{new InputStream()};
  template <typename T>
  T get() const {
    T res;
    _stream->read(reinterpret_cast<char *>(&res), sizeof(res));
    return res;
  }
 public:
  BinaryInputArchive(InputStream &stream) {
    _stream->swap(stream);
  }
  bool getBool(const char *) const { return get<bool>(); }
  int getInt(const char *) const { return get<int>(); }
  uint32_t getUint(const char *) const { return get<uint32_t>(); }
  uint64_t getUint64(const char *) const { return get<uint64_t>(); }
  double getDouble(const char *) const { return get<double>(); }
  float getFloat(const char *) const { return get<float>(); }
  std::string getString(const char *) const {
    size_t len = get<size_t>();
    std::string res;
    res.resize(len);
    _stream->read(const_cast<char *>(res.c_str()), len);
    return res;
  }
  template <typename T>
  std::vector<T *> getObjects(const char *) const {
    std::vector<T *> res;
    size_t len = get<size_t>();
    for(auto i = 0; i < len; ++i) {
      res.push_back(new T{*this});
    }
    return res;
  }
  template <typename T, typename OutputIter>
  void getObjects(const char *key, OutputIter out) const {
    size_t len = get<size_t>();
    for(auto i = 0; i < len; ++i) {
      *out++ = get<T>();
    } 
  }
  void parseObjects(const char *, const std::function<void(const BinaryInputArchive &)>&task) const {
    size_t len = get<size_t>();
    for(auto i = 0; i < len; ++i) {
      task(*this);
    }
  }
  BinaryInputArchive getObject(const char *) const { return *this; }
};

