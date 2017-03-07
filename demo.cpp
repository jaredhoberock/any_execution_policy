#include <agency/agency.hpp>
#include "async_reduce/reduce.hpp"
#include "any_execution_policy.hpp"
#include <iostream>

int main()
{
  any_execution_policy policy = agency::par;

  assert(policy.type() == typeid(agency::par));

  std::cout << "OK" << std::endl;

  return 0;
}

