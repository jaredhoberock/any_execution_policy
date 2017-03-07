#pragma once

#include <agency/agency.hpp>
#include <experimental/any>
#include <utility>

class any_execution_policy
{
  public:
    template<class ExecutionPolicy,
             __AGENCY_REQUIRES(agency::is_execution_policy<typename std::decay<ExecutionPolicy>::type>::value)>
    any_execution_policy(ExecutionPolicy&& policy)
      : policy_(std::forward<ExecutionPolicy>(policy))
    {}

    const std::type_info& type() const
    {
      return policy_.type();
    }

  private:
    std::experimental::any policy_;
};

