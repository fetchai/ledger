#include "oef-base/monitoring/Monitoring.hpp"

#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>

Monitoring::MonitoringInner *Monitoring::inner = 0;

Monitoring::Monitoring()
{
  if (!inner)
  {
    inner = new MonitoringInner;
  }
}

Monitoring::~Monitoring()
{}

Monitoring::IdType Monitoring::find(const NameType &name)
{
  if (!inner)
  {
    inner = new MonitoringInner;
  }
  return inner->get(name);
}

void Monitoring::add(IdType id, CountType delta)
{
  inner->access(id) += delta;
}

void Monitoring::set(IdType id, CountType delta)
{
  inner->access(id) = delta;
}

void Monitoring::sub(IdType id, CountType delta)
{
  inner->access(id) -= delta;
}

void Monitoring::report(ReportFunc func)
{
  for (const auto &name2id : inner->GetNames())
  {
    func(name2id.first, inner->access(name2id.second));
  }
}

void Monitoring::max(IdType id, CountType value)
{}

Monitoring::CountType Monitoring::get(IdType id)
{
  return inner->access(id);
}
