#pragma once

#include <utility>
#include <agency/agency.hpp>

// XXX should probably make variant public like optional
#include <agency/detail/variant.hpp>


template<class ExecutionAgent, class... ExecutionAgents>
class variant_execution_agent
{
  public:
    template<class Agent>
    variant_execution_agent(Agent& self)
      : variant_(std::move(self))
    {}

  private:
    struct index_visitor
    {
      template<class Agent>
      std::size_t index(const Agent& self) const
      {
        return self.index();
      }
    };

  public:
    std::size_t index() const
    {
      return agency::detail::visit(index_visitor(), variant_);
    }

  private:
    struct group_shape_visitor
    {
      template<class Agent>
      std::size_t group_shape(const Agent& self) const
      {
        return self.group_shape();
      }
    };

  public:
    std::size_t group_shape() const
    {
      return agency::detail::visit(group_shape_visitor(), variant_);
    }

  private:
    using variant_type = agency::detail::variant<ExecutionAgent,ExecutionAgents...>;

    variant_type variant_;
};


template<class ExecutionPolicy, class... ExecutionPolicies>
class variant_execution_policy
{
  public:
    using execution_agent_type = variant_execution_agent<
      typename ExecutionPolicy::execution_agent_type,
      typename ExecutionPolicies::execution_agent_type...
    >;

    template<class Policy,
             __AGENCY_REQUIRES(agency::is_execution_policy<typename std::decay<Policy>::type>::value)
            >
    variant_execution_policy(Policy&& policy)
      : variant_(std::forward<Policy>(policy))
    {}

    std::size_t index() const
    {
      return variant_.index();
    }

    variant_execution_policy operator()(size_t n) const
    {
      return agency::detail::visit(call_operator_visitor{n}, variant_);
    }

    // XXX just return sequenced_executor to satisfy is_execution_policy, which requires .executor() exist
    //     should either implement variant_executor or allow execution policies to not have .executor()
    //     the idea being that is_execution_policy<T> is true if either
    agency::sequenced_executor executor() const
    {
      return agency::sequenced_executor();
    }

  //private:
    struct call_operator_visitor
    {
      size_t n;

      template<class Policy>
      variant_execution_policy operator()(const Policy& policy) const
      {
        return variant_execution_policy(policy(n));
      }
    };

    using variant_type = agency::detail::variant<ExecutionPolicy,ExecutionPolicies...>;

    variant_type variant_;
};


namespace detail
{


template<class Function, class... Args>
struct bulk_invoke_visitor
{
  Function f;
  agency::detail::tuple<Args...> args;

  template<class ExecutionPolicy, size_t... Indices>
  auto impl(ExecutionPolicy&& policy, agency::detail::index_sequence<Indices...>) const ->
    decltype(agency::bulk_invoke(std::forward<ExecutionPolicy>(policy), f, agency::detail::get<Indices>(args)...))
  {
    return agency::bulk_invoke(std::forward<ExecutionPolicy>(policy), f, agency::detail::get<Indices>(args)...);
  }

  template<class ExecutionPolicy>
  auto operator()(ExecutionPolicy&& policy) const ->
    decltype(impl(std::forward<ExecutionPolicy>(policy), agency::detail::make_tuple_indices(args)))
  {
    return impl(std::forward<ExecutionPolicy>(policy), agency::detail::make_tuple_indices(args));
  }
};


template<class Function, class... Args>
bulk_invoke_visitor<Function, Args...> make_bulk_invoke_visitor(Function f, Args&&... args)
{
  return bulk_invoke_visitor<Function,Args...>{f, agency::detail::forward_as_tuple(std::forward<Args>(args)...)};
}


template<class Function, class ListOfPossibleAgents, class... Args>
struct is_call_possible_with_all_possible_agents;

template<class Function, class... ExecutionAgents, class... Args>
struct is_call_possible_with_all_possible_agents<Function, agency::detail::type_list<ExecutionAgents...>, Args...>
  : agency::detail::conjunction<
      agency::detail::is_call_possible<
        Function, ExecutionAgents&, Args...
      >...
    >
{};


} // end detail


// these requirements check whether visitor(first_agent_type, args...) is possible
// if it is, then we assume that the same is true of all the other possible agent types
template<class ExecutionPolicy, class... ExecutionPolicies, class AgentVisitor, class... Args,
         __AGENCY_REQUIRES(agency::detail::is_bulk_call_possible_via_execution_policy<
                             ExecutionPolicy, AgentVisitor, Args...
                           >::value)
        >
agency::detail::bulk_invoke_execution_policy_result_t<
  ExecutionPolicy,
  AgentVisitor,
  Args...
>
  bulk_invoke(const variant_execution_policy<ExecutionPolicy,ExecutionPolicies...>& policy, AgentVisitor visitor, Args&&... args)
{
  return agency::detail::visit(detail::make_bulk_invoke_visitor(visitor, std::forward<Args>(args)...), policy.variant_);
}


// these requirements check whether
// 1. the above overload is not possible and
// 2. f(variant_execution_agent, args...) is possible
// if so, then we wrap f to make it accept all possible agent types and lower onto the above overload of ::bulk_invoke()
template<class ExecutionPolicy, class... ExecutionPolicies, class Function, class... Args,
         __AGENCY_REQUIRES(!agency::detail::is_bulk_call_possible_via_execution_policy<
                             ExecutionPolicy, Function, Args...
                           >::value),
         __AGENCY_REQUIRES(agency::detail::is_bulk_call_possible_via_execution_policy<
                             variant_execution_policy<ExecutionPolicy,ExecutionPolicies...>,
                             Function,
                             Args...
                           >::value)
        >
agency::detail::bulk_invoke_execution_policy_result_t<
  variant_execution_policy<ExecutionPolicy,ExecutionPolicies...>,
  Function,
  Args...
>
  bulk_invoke(const variant_execution_policy<ExecutionPolicy,ExecutionPolicies...>& policy, Function f, Args&&... args)
{
  using variant_agent_type = typename variant_execution_policy<ExecutionPolicy,ExecutionPolicies...>::execution_agent_type;

  // wrap f in a visitor that can accept any type of agent
  auto visitor = [=](auto& self, auto&&... args)
  {
    // erase the type of self
    variant_agent_type type_erased_self(self);

    // invoke f with the type erased agent
    return f(type_erased_self, args...);
  };

  return ::bulk_invoke(policy, visitor, std::forward<Args>(args)...);
}

