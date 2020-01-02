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

#include "MtSearch.hpp"

#include <fstream>
#include <iostream>

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/Endpoint.hpp"
#include "oef-base/comms/IOefListener.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/monitoring/Monitoring.hpp"
#include "oef-base/threading/MonitoringTask.hpp"
#include "oef-base/utils/Uri.hpp"

#include "google/protobuf/util/json_util.h"
#include "oef-search/comms/OefListenerStarterTask.hpp"
#include "oef-search/comms/OefSearchEndpoint.hpp"
#include "oef-search/dap_comms/OutboundDapConversationCreator.hpp"
#include "oef-search/functions/DirectorTaskFactory.hpp"
#include "oef-search/search_comms/OutboundSearchConversationCreator.hpp"

#include <ctype.h>
#include <stdio.h>

using namespace std::placeholders;

static const unsigned int minimum_thread_count = 1;

std::string prometheusUpThatNamingString(const std::string &name)
{
  std::string r;
  bool        upshift = false;
  for (std::size_t i = 0; i < name.length(); i++)
  {
    auto c = name[i];

    switch (c)
    {
    case '-':
    case '_':
      upshift = true;
      break;
    case '.':
      r += "_";
      break;

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':

    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':

    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':

    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':

    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':

    case 'z':
      if (upshift)
      {
        r += std::string(1, static_cast<char>(::toupper(c)));
        upshift = false;
        break;
      }

    /* fall through */
    default:
      r += c;
      break;
    }
  }
  return r;
}

void MtSearch::AddPeer(const std::string &peer_uri)
{
  Uri uri(peer_uri);
  search_peer_store_->AddPeer(peer_uri);
  outbounds->AddConversationCreator(
      Uri(peer_uri), std::make_shared<OutboundSearchConversationCreator>(uri, *core));
}

int MtSearch::run()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting search...");
  FETCH_LOG_INFO(LOGGING_NAME, "Search key: ", config_.search_key());
  FETCH_LOG_INFO(LOGGING_NAME, "Search URI: ", config_.search_uri());
  FETCH_LOG_INFO(LOGGING_NAME, "comms_thread_count: ", config_.comms_thread_count());
  FETCH_LOG_INFO(LOGGING_NAME, "tasks_thread_count: ", config_.tasks_thread_count());
  FETCH_LOG_INFO(LOGGING_NAME, "Search config: ", config_.DebugString());

  core    = std::make_shared<Core>();
  auto ts = std::make_shared<fetch::oef::base::Taskpool>();
  ts->SetDefault();
  outbounds = std::make_shared<OutboundConversations>();
  listeners =
      std::make_shared<OefListenerSet<IOefTaskFactory<OefSearchEndpoint>, OefSearchEndpoint>>();

  dap_store_         = std::make_shared<DapStore>();
  search_peer_store_ = std::make_shared<SearchPeerStore>();
  dap_manager_       = std::make_shared<DapManager>(dap_store_, search_peer_store_, outbounds,
                                              config_.query_cache_lifetime_sec());

  for (const auto &dap_config : config_.daps())
  {
    Uri uri(dap_config.uri());
    dap_store_->AddDap(dap_config.name());
    outbounds->AddConversationCreator(
        Uri("dap://" + dap_config.name() + ":0"),
        std::make_shared<OutboundDapConversationCreator>(uri, *core, dap_config.name()));
  }
  for (const auto &peer_uri : config_.peers())
  {
    AddPeer(peer_uri);
  }

  dap_manager_->setup();

  std::function<void(void)>                      run_comms = std::bind(&Core::run, core.get());
  std::function<void(std::size_t thread_number)> run_tasks =
      std::bind(&fetch::oef::base::Taskpool::run, tasks.get(), _1);

  comms_runners.start(std::max(config_.comms_thread_count(), minimum_thread_count), run_comms);
  tasks_runners.start(std::max(config_.tasks_thread_count(), minimum_thread_count), run_tasks);

  startListeners();

  Monitoring mon;
  auto       mon_task = std::make_shared<fetch::oef::base::MonitoringTask>();
  mon_task->submit();

  std::map<std::string, std::string> prometheus_names;

  while (1)
  {
    tasks->UpdateStatus();

    unsigned int snooze = 3;

    if (config_.prometheus_log_file().length())
    {
      if (config_.prometheus_log_interval())
      {
        snooze = config_.prometheus_log_interval();
      }

      std::string final_file = config_.prometheus_log_file();
      std::string temp_file  = final_file + ".tmp";

      std::fstream fs;
      fs.open(temp_file.c_str(), std::fstream::out);
      if (fs.is_open())
      {
        mon.report([&fs, &prometheus_names](const std::string &name, std::size_t value) {
          std::string new_name;
          auto        new_name_iter = prometheus_names.find(name);
          if (new_name_iter == prometheus_names.end())
          {
            new_name               = prometheusUpThatNamingString(name);
            prometheus_names[name] = new_name;
          }
          else
          {
            new_name = new_name_iter->second;
          }

          if (new_name.find("_gauge_") != std::string::npos)
          {
            fs << "# TYPE " << new_name << " gauge" << std::endl;
          }
          else
          {
            new_name += "_total";
            fs << "# TYPE " << new_name << " counter" << std::endl;
          }
          fs << new_name << " " << value << std::endl;
        });

        if (::rename(temp_file.c_str(), final_file.c_str()) != 0)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Could not create ", final_file);
        }
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Could not create ", temp_file);
      }
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "----------------------------------------------");
      mon.report([](const std::string &name, std::size_t value) {
        FETCH_LOG_INFO(LOGGING_NAME, name, ":", value);
      });
    }
    sleep(snooze);
  }
  return 0;
}

void MtSearch::startListeners()
{
  auto                    this_sp = this->shared_from_this();
  std::weak_ptr<MtSearch> this_wp = this_sp;

  IOefListener<IOefTaskFactory<OefSearchEndpoint>,
               OefSearchEndpoint>::FactoryCreator initialFactoryCreator =
      [this_wp](std::shared_ptr<OefSearchEndpoint> endpoint) -> std::shared_ptr<SearchTaskFactory> {
    auto sp = this_wp.lock();
    if (sp)
    {
      return std::make_shared<SearchTaskFactory>(std::move(endpoint), sp->outbounds,
                                                 sp->dap_manager_);
    }
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "Can't create SearchTaskFactory because can't lock week pointer!");
    return nullptr;
  };

  Uri search_uri(config_.search_uri());
  FETCH_LOG_INFO(LOGGING_NAME, "Listener on ", search_uri.port);
  std::unordered_map<std::string, std::string> endpointConfig;

  auto task = std::make_shared<OefListenerStarterTask<Endpoint>>(
      search_uri.port, listeners, core, initialFactoryCreator, endpointConfig);
  task->submit();

  if (!config_.director_uri().empty())
  {
    Uri director_uri(config_.director_uri());
    FETCH_LOG_INFO(LOGGING_NAME, "Director listener started on ", director_uri.port);

    auto directorFactoryCreator = [this_wp](std::shared_ptr<OefSearchEndpoint> endpoint)
        -> std::shared_ptr<IOefTaskFactory<OefSearchEndpoint>> {
      auto sp = this_wp.lock();
      if (sp)
      {
        return std::make_shared<DirectorTaskFactory>(std::move(endpoint), sp->outbounds,
                                                     sp->dap_manager_, sp->config_, sp);
      }
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Can't create DirectorTaskFactory because can't lock week pointer!");
      return nullptr;
    };

    auto d_task = std::make_shared<OefListenerStarterTask<Endpoint>>(
        director_uri.port, listeners, core, directorFactoryCreator, endpointConfig);
    d_task->submit();
  }
}

bool MtSearch::configure(const std::string &config_file, const std::string &config_json)
{
  if (config_file.length())
  {
    return configureFromJsonFile(config_file);
  }
  if (config_json.length())
  {
    return configureFromJson(config_json);
  }
  return false;
}

bool MtSearch::configureFromJsonFile(const std::string &config_file)
{
  std::ifstream json_file(config_file);

  std::string json = "";
  if (!json_file.is_open())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to load configuration: '" + config_file + "'");
    return false;
  }
  else
  {
    std::string line;
    while (std::getline(json_file, line))
    {
      json += line;
    }
  }
  return configureFromJson(json);
}

bool MtSearch::configureFromJson(const std::string &config_json)
{
  google::protobuf::util::JsonParseOptions options;
  options.ignore_unknown_fields = true;

  auto status = google::protobuf::util::JsonStringToMessage(config_json, &config_, options);
  if (!status.ok())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Parse error: '" + status.ToString() + "'");
  }
  return status.ok();
}
