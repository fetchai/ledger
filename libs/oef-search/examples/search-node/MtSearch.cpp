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
#include "oef-base/comms/OefListenerSet.hpp"

#include "oef-base/monitoring/Monitoring.hpp"
#include "oef-base/threading/MonitoringTask.hpp"

#include "oef-base/utils/Uri.hpp"

#include "oef-messages/fetch_protobuf.hpp"
#include "oef-search/comms/OefListenerStarterTask.hpp"
#include "oef-search/comms/OefSearchEndpoint.hpp"
#include "oef-search/dap_comms/OutboundDapConversationCreator.hpp"
#include <stdio.h>

#include <ctype.h>

using namespace std::placeholders;

static const unsigned int minimum_thread_count = 1;

std::string prometheusUpThatNamingString(const std::string &name)
{
  std::string r;
  bool        upshift = false;
  for (int i = 0; i < name.length(); i++)
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
        r += std::string(1, ::toupper(c));
        upshift = false;
        break;
      }
    default:
      r += c;
      break;
    }
  }
  return r;
}

int MtSearch::run()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Starting search...");
  FETCH_LOG_INFO(LOGGING_NAME, "Search key: ", config_.search_key());
  FETCH_LOG_INFO(LOGGING_NAME, "Search URI: ", config_.search_uri());
  FETCH_LOG_INFO(LOGGING_NAME, "comms_thread_count: ", config_.comms_thread_count());
  FETCH_LOG_INFO(LOGGING_NAME, "tasks_thread_count: ", config_.tasks_thread_count());

  core       = std::make_shared<Core>();
  auto tasks = std::make_shared<Taskpool>();
  tasks->SetDefault();
  outbounds = std::make_shared<OutboundConversations>();
  listeners = std::make_shared<OefListenerSet<SearchTaskFactory, OefSearchEndpoint>>();

  std::size_t thread_group_id = 1500;
  for (const auto &dap_config : config_.daps())
  {
    ++thread_group_id;
    Uri uri(dap_config.uri());
    outbounds->AddConversationCreator(
        dap_config.name(),
        std::make_shared<OutboundDapConversationCreator>(thread_group_id, uri, *core, outbounds));
  }

  std::function<void(void)>                      run_comms = std::bind(&Core::run, core.get());
  std::function<void(std::size_t thread_number)> run_tasks =
      std::bind(&Taskpool::run, tasks.get(), _1);

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
  IOefListener<SearchTaskFactory, OefSearchEndpoint>::FactoryCreator initialFactoryCreator =
      [this](std::shared_ptr<OefSearchEndpoint> endpoint) {
        return std::make_shared<SearchTaskFactory>(endpoint, outbounds);
      };

  Uri search_uri(config_.search_uri());
  FETCH_LOG_INFO(LOGGING_NAME, "Listener on ", search_uri.port);
  std::unordered_map<std::string, std::string> endpointConfig;

  auto task = std::make_shared<OefListenerStarterTask<Endpoint>>(
      search_uri.port, listeners, core, initialFactoryCreator, endpointConfig);
  task->submit();
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
