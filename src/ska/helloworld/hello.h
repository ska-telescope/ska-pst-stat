#ifndef SKA_HELLOWORLD_HELLO_H
#define SKA_HELLOWORLD_HELLO_H
#include <string>

namespace ska {
namespace helloworld {

class hello {

  public:
    hello();
    ~hello();

    virtual std::string greeting() = 0;

};
} // namespace helloworld
} // namespace ska
#endif // SKA_HELLOWORLD_HELLO_H
