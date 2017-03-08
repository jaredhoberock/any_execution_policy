#include <agency/agency.hpp>
#include "async_reduce/reduce.hpp"
#include "variant_execution_policy.hpp"
#include <iostream>
#include <atomic>


using any_policy = variant_execution_policy<agency::sequenced_execution_policy, agency::parallel_execution_policy>;
using any_agent = any_policy::execution_agent_type;


struct counting_visitor
{
  std::atomic<int>& counter;
  std::mutex& mutex;

  void operator()(agency::parallel_agent& self) const
  {
    mutex.lock();
    std::cout << "Hello world from parallel_agent " << self.index() << std::endl;
    mutex.unlock();

    ++counter;
  }

  void operator()(agency::sequenced_agent& self) const
  {
    std::cout << "Hello world from sequenced_agent " << self.index() << std::endl;

    ++counter;
  }
};

int main()
{
  {
    any_policy policy = agency::seq;
    assert(policy.index() == 0);

    policy = agency::par;
    assert(policy.index() == 1);
  }

  {
    std::mutex mutex;
    std::atomic<int> counter;

    counter = 0;
    any_policy policy = agency::seq;
    ::bulk_invoke(policy(10), [&](any_agent& self)
    {
      ++counter;
    });
    assert(counter == 10);


    counter = 0;
    policy = agency::par;
    ::bulk_invoke(policy(10), [&](any_agent& self)
    {
      ++counter;
    });
    assert(counter == 10);
  }

  {
    std::mutex mutex;
    std::atomic<int> counter;

    any_policy policy = agency::seq;

    counter = 0;
    ::bulk_invoke(policy(10), counting_visitor{counter, mutex});
    assert(counter == 10);

    policy = agency::par;

    counter = 0;
    ::bulk_invoke(policy(10), counting_visitor{counter, mutex});
    assert(counter == 10);
  }

  std::cout << "OK" << std::endl;

  return 0;
}

