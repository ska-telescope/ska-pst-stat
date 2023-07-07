#ifndef SKA_HELLOWORLD_WAVE_IMPL_H
#define SKA_HELLOWORLD_WAVE_IMPL_H

#include <string>

#include "ska/helloworld/wave.h"

namespace ska {
namespace helloworld {

class wave::wave_impl {
public:
  wave_impl();
  ~wave_impl();
  std::string greeting();
};
} // namespace helloworld
} // namespace ska

#endif // SKA_HELLOWORLD_WAVE_IMPL_H
