#ifndef MARINATB_CORE_COMPILATION_HXX
#define MARINATB_CORE_COMPILATION_HXX

#include "blueprint.hxx"

namespace marina
{
  class Diagnostics
  {
    public:
      Blueprint blueprint();
      Json source();

    private:
      Blueprint blueprint_;
      Json source_;
  };
}

#endif
