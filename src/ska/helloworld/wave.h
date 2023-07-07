#ifndef SKA_HELLOWORLD_WAVE_H
#define SKA_HELLOWORLD_WAVE_H

#include <memory>
#include <string>

#include "ska/helloworld/hello.h"

namespace ska {
namespace helloworld {

/// A wave class implemented with a Pimpl idiom
class wave : public hello {
public:
  wave();
  ~wave();
  std::string greeting() override;

private:
  class wave_impl;
  std::unique_ptr<wave_impl> impl;
};

} // namespace helloworld
} // namespace ska

#endif // SKA_HELLOWORLD_WAVE_H
