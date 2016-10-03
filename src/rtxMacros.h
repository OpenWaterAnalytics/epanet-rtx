//
//  rtxMacros.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_rtxMacros_h
#define epanet_rtx_rtxMacros_h

#include <iostream>
#include <memory>

#include "RTX_Visitable.h"

#define RTX_BASE_PROPS(type) typedef std::shared_ptr<type> _sp; RTX_VISITABLE()

namespace RTX {
  class RTX_object : public RTX_VISITABLE_TYPE {
  public:
    RTX_BASE_PROPS(RTX_object);
  };
}

#define RTX_MAX_CHAR_STRING 256
#define RTX_MAX(x,y) x>y?x:y
#define RTX_MIN(x,y) x<y?x:y

// silly macros for silly string comparison
#include <boost/algorithm/string/predicate.hpp>
#define RTX_STRINGS_ARE_EQUAL(x,y) (boost::algorithm::iequals(x,y))
#define RTX_STRINGS_ARE_EQUAL_CS(x,y) (x.compare(y)==0)


#ifndef RTX_BUFFER_DEFAULT_CACHESIZE
#define RTX_BUFFER_DEFAULT_CACHESIZE 100
#endif

#define DEBUG 1
#ifdef DEBUG
#define DebugLog cout
#else
#define DebugLog if(false) cout
#endif

#define EOL '\n'


#endif
