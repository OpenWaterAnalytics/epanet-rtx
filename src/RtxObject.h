#ifndef epanet_rtx_object_h
#define epanet_rtx_object_h

#include "rtxMacros.h"

namespace RTX {
  class RTX_object : public TSF::TSF_object {
  public:
    TSF_BASE_PROPS(RTX_object);
  };

  typedef RTX_object::_sp RTX_ptr;
}

#endif