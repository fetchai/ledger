#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "Leaf.hpp"
#include "core/mutex.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-messages/dap_interface.hpp"

#include <functional>
#include <google/protobuf/repeated_field.h>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class DapStore
{
public:
  static constexpr char const *LOGGING_NAME = "DapStore";

  DapStore()
  {
    SetupFilters();
  }

  virtual ~DapStore() = default;

  void AddDap(const std::string &name)
  {
    FETCH_LOCK(mutex_);
    daps_.push_back(name);
  }

  const std::vector<std::string> &GetDaps() const
  {
    return daps_;
  }

  bool CheckOption(const std::string &                                    option,
                   const google::protobuf::RepeatedPtrField<std::string> &options) const
  {
    for (const auto &op : options)
    {
      if (option == op)
      {
        return true;
      }
    }
    return false;
  }

  void ConfigureDap(const std::string &dap_name, DapDescription const &config)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Configure: ", dap_name);
    std::lock_guard<std::mutex>                                                 lock(mutex_);
    std::shared_ptr<std::pair<std::string, DapDescription_DapFieldDescription>> plane_description =
        nullptr;
    for (const auto &table_desc : config.table())
    {
      for (const auto &field_desc : table_desc.field())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "--> Add field description: ", field_desc.type());
        attributes_to_dapnames_[field_desc.name()].push_back(dap_name);
        if (CheckOption("replace_target_info", field_desc.options()))
        {
          FETCH_LOG_INFO(LOGGING_NAME, "target_query_type_to_tbandfield_name_: ", field_desc.type(),
                         " set field: ", field_desc.name());
          target_query_type_to_tbandfield_name_[field_desc.type()] =
              std::make_pair(table_desc.name(), field_desc.name());
        }
        if (CheckOption("plane", field_desc.options()))
        {
          if (plane_description == nullptr)
          {
            plane_description =
                std::make_shared<std::pair<std::string, DapDescription_DapFieldDescription>>(
                    std::make_pair(table_desc.name(),
                                   DapDescription_DapFieldDescription(field_desc)));
          }
          else
          {
            FETCH_LOG_WARN(LOGGING_NAME, "Dap ", dap_name,
                           " has multiple plane fields! Only one supported!");
          }
        }
      }
    }
    for (const std::string &option : config.options())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Add option: ", option, " to dap ", dap_name);
      dap_options_[dap_name].insert(option);
    }
    ++configured_daps_;
    if (IsDap(dap_name, "geo"))
    {
      if (geo_dap_.empty())
      {
        geo_dap_                   = dap_name;
        plane_descriptions_["geo"] = plane_description;
        if (plane_description == nullptr)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "GEO dap ", dap_name,
                         " does not have plane decorated field. Search broadcasting won't work!");
        }
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Geo dap already provided (", geo_dap_,
                        ")! Multiple geo daps not supported!", " Ignoring: ", dap_name);
      }
    }
    if (configured_daps_ == daps_.size() && geo_dap_.empty())
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "All DAPs configured, but no geo dap is provided (dap with option 'geo')! ",
                     "Location based services might not work correctly!");
    }
  }

  std::vector<std::string> GetDapsForAttributeType(const std::string &type) const
  {
    auto                     it = target_query_type_to_tbandfield_name_.find(type);
    std::vector<std::string> daps{};
    if (it != target_query_type_to_tbandfield_name_.end())
    {
      auto it2 = attributes_to_dapnames_.find(it->second.second);
      if (it2 != attributes_to_dapnames_.end())
      {
        daps = it2->second;
      }
    }
    return daps;
  }

  std::unordered_set<std::string> GetDapsForAttributeName(const std::string &attributeName) const
  {
    std::unordered_set<std::string> dap_names_tmp_{};

    std::regex them_re("^them\\.");
    auto       attr_name_no_them = std::regex_replace(attributeName, them_re, "");

    std::vector<std::pair<std::string, std::string>> attrs = {
        {attr_name_no_them, "always_true"},
        {attr_name_no_them + ".update", "always_true"},
        {attributeName + ".update", "always_true"},
        {"them." + attributeName, "not_lazy"},
        {"them." + attr_name_no_them, "lazy_no_res"}};

    for (const auto &attr_pair : attrs)
    {
      for (const auto &dap : attributes_to_dapnames_)
      {
        if (!MatchAttributeName(dap.first, attr_pair.first))
        {
          continue;
        }
        for (const auto &dapname : dap.second)
        {
          auto it = dap_filters_.find(attr_pair.second);
          if (it != dap_filters_.end() && it->second(dapname, dap_names_tmp_))
          {
            dap_names_tmp_.insert(dapname);
          }
        }
      }
    }

    return dap_names_tmp_;
  }

  bool MatchAttributeName(const std::string &attribute_pattern,
                          const std::string &attribute_name) const
  {
    if (attribute_pattern == "*")
    {
      return true;
    }
    if (attribute_name == "*")
    {
      return true;
    }
    if (attribute_pattern.front() == '/' && attribute_pattern.back() == '/')
    {
      std::regex re("^" + attribute_pattern.substr(1, attribute_pattern.length() - 2) + "$");
      return std::regex_search(attribute_name, re);
    }
    if (attribute_pattern == attribute_name)
    {
      return true;
    }
    return false;
  }

  bool IsDap(const std::string &dap_name, const std::string &attribute) const
  {
    bool value = false;
    auto it    = dap_options_.find(dap_name);
    if (it != dap_options_.end())
    {
      value = it->second.find(attribute) != it->second.end();
    }
    return value;
  }

  std::vector<std::string> GetDapNamesByOptions(std::vector<std::string> const &attributes) const
  {
    std::vector<std::string> daps{};
    for (const auto &e : dap_options_)
    {
      for (const auto &attr : attributes)
      {
        if (e.second.find(attr) != e.second.end())
        {
          daps.push_back(e.first);
        }
      }
    }
    return daps;
  }

  bool IsDapEarly(const std::string &dap_name) const
  {
    return IsDap(dap_name, "early");
  }

  bool IsDapLate(const std::string &dap_name) const
  {
    return IsDap(dap_name, "late");
  }

  void UpdateTargetFieldAndTableNames(Leaf &leaf) const
  {
    auto it = target_query_type_to_tbandfield_name_.find(leaf.GetQueryFieldType());
    if (it != target_query_type_to_tbandfield_name_.end())
    {
      leaf.SetTargetTableName(it->second.first);
      leaf.SetTargetFieldName(it->second.second);
    }
  }

  void UpdateTargetFieldAndTableNames(ConstructQueryConstraintObjectRequest &c) const
  {
    auto it = target_query_type_to_tbandfield_name_.find(c.query_field_type());
    if (it != target_query_type_to_tbandfield_name_.end())
    {
      c.set_target_table_name(it->second.first);
      c.set_target_field_name(it->second.second);
    }
  }

  const std::string &GetGeoDap() const
  {
    return geo_dap_;
  }

  std::shared_ptr<std::pair<std::string, DapDescription_DapFieldDescription>> GetPlaneDescription(
      const std::string &plane)
  {
    auto it = plane_descriptions_.find(plane);
    if (it != plane_descriptions_.end())
    {
      return it->second;
    }
    return nullptr;
  }

protected:
  std::unordered_map<std::string, std::vector<std::string>>        attributes_to_dapnames_{};
  std::unordered_map<std::string, std::unordered_set<std::string>> dap_options_{};
  std::vector<std::string>                                         daps_{};
  std::mutex                                                       mutex_;
  std::string                                                      geo_dap_{};
  uint32_t                                                         configured_daps_ = 0;

  std::unordered_map<std::string, std::pair<std::string, std::string>>
      target_query_type_to_tbandfield_name_;
  std::unordered_map<std::string,
                     std::shared_ptr<std::pair<std::string, DapDescription_DapFieldDescription>>>
      plane_descriptions_;

  std::unordered_map<std::string, std::function<bool(const std::string &,
                                                     const std::unordered_set<std::string> &)>>
      dap_filters_;

  void SetupFilters()
  {
    dap_filters_["always_true"] =
        [](const std::string &, const std::unordered_set<std::string> &) -> bool { return true; };
    dap_filters_["not_lazy"] = [this](const std::string &dap,
                                      const std::unordered_set<std::string> &) -> bool {
      return !IsDap(dap, "lazy");
    };
    dap_filters_["lazy_no_res"] = [this](const std::string &                    dap,
                                         const std::unordered_set<std::string> &daps) -> bool {
      return daps.empty() && IsDap(dap, "lazy");
    };
  }

private:
  DapStore(const DapStore &other) = delete;              // { copy(other); }
  DapStore &operator=(const DapStore &other)  = delete;  // { copy(other); return *this; }
  bool      operator==(const DapStore &other) = delete;  // const { return compare(other)==0; }
  bool      operator<(const DapStore &other)  = delete;  // const { return compare(other)==-1; }
};
