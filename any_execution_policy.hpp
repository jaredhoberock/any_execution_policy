#pragma once

#include <agency/agency.hpp>
#include <agency/experimental.hpp>
#include <experimental/any>
#include <utility>
#include <functional>

class any_view
{
  public:
    template<class Range>
    any_view(const Range& rng)
      : bracket_([](size_t i)
    {
      return std::experimental::any(std::ref(rng[i]));
    })
    {}

    std::experimental::any operator[](size_t i) const
    {
      return bracket_(i);
    }

  private:
    std::function<std::experimental::any(size_t i)> bracket_;
};

class any_binary_operator
{
  public:
    template<class BinaryOperator>
    any_binary_operator(const BinaryOperator& binary_op)
      : function_(binary_op)
    {}

    std::experimental::any operator(const std::experimental::any& a, const std::experimental::any& b) const
    {
      return function_(a, b);
    }

  private:
    std::function<std::experimental::any(const std::experimental::any&, const std::experimental::any&)> function_;
};

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

    // XXX the type erasure going on here looks really expensive
    any reduce(any_view rng, std::experimental::any init, any_binary_operator binary_op) const;

  private:
    std::experimental::any policy_;
};

