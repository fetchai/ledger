#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "datum.hpp"
#include "depot.hpp"
#include "target.hpp"
#include "vehicle.hpp"

#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <vector>

namespace mobi {

class World
{
public:
  using DepotType   = std::map<std::size_t, Depot>;
  using VehicleType = std::map<std::size_t, Vehicle>;
  using TargetType  = std::map<std::size_t, Target>;

  /**
   * Constructor sets up depots, vehicles, and targets
   * @param n_depots
   * @param n_vehicles
   * @param n_targets
   */
  World(std::size_t n_depots = 1, std::size_t n_vehicles = 3, std::size_t n_targets = 25)
  {
    // First add all depots to the world
    for (std::size_t i = 0; i < n_depots; ++i)
    {
      depots_.insert(std::pair<std::size_t, Depot>(datum_counter, Depot()));
      ++datum_counter;
    }

    // Next add all vehicles to the world
    for (std::size_t i = 0; i < n_vehicles; ++i)
    {
      vehicles_.insert(std::pair<std::size_t, Vehicle>(
          datum_counter, Vehicle(depots_[0].longitude, depots_[1].latitude)));
      ++datum_counter;
    }

    // Last add all targets to the world

    double long_max = depots_[0].longitude + 0.1;
    double long_min = depots_[0].longitude - 0.1;
    double lat_max  = depots_[0].latitude + 0.1;
    double lat_min  = depots_[0].latitude - 0.1;

    double rand_longitude;
    double rand_latitude;

    std::uniform_real_distribution<double> long_rand_gen(long_min, long_max);
    std::uniform_real_distribution<double> lat_rand_gen(lat_min, lat_max);
    std::default_random_engine             re;

    for (std::size_t i = 0; i < n_targets; ++i)
    {
      rand_longitude = long_rand_gen(re);
      rand_latitude  = lat_rand_gen(re);

      targets_.insert(
          std::pair<std::size_t, Target>(datum_counter, Target(rand_longitude, rand_latitude)));
      ++datum_counter;
    }
  }

  /**
   * returns all data about the world
   * @return
   */
  DepotType GetDepots()
  {
    return depots_;
  }

  VehicleType GetVehicles()
  {
    return vehicles_;
  }

  TargetType GetTargets()
  {
    return targets_;
  }

private:
  std::size_t datum_counter = 0;
  DepotType   depots_;
  VehicleType vehicles_;
  TargetType  targets_;
};

}  // namespace mobi
