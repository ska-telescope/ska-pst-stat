/// System Includes
#include <iostream>
#include <string>

/// Local Includes
#include "ska/helloworld/wave.h"
#include "ska/helloworld/wave_impl.h"

namespace ska {
namespace helloworld {

wave::wave_impl::wave_impl() {
  std::cout << "The derived(virtual) constructor for a wave" << std::endl;
}

wave::wave_impl::~wave_impl() {
  std::cout << "The derived(virtual) destructor for a wave" << std::endl;
}

std::string wave::wave_impl::greeting() {
  return std::string("I am waving hello");
}

wave::wave()
 : impl(std::make_unique<wave_impl>())
{
}

wave::~wave() = default;

std::string wave::greeting()
{
  return impl->greeting();
}


} // namespace helloworld
} // namespace ska
