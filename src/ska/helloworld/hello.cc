/// System Includes
#include <iostream>
/// Local Includes
#include "ska/helloworld/hello.h"
namespace ska {
namespace helloworld {
hello::hello() {
  std::cout << "The default constructor for a hello" << std::endl;
}
hello::~hello() {
  std::cout << "The default destructor for a hello" << std::endl;
}
} // namespace helloworld
} // namespace ska
