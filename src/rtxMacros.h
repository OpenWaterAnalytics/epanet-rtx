//
//  rtxMacros.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_rtxMacros_h
#define epanet_rtx_rtxMacros_h

#include <boost/shared_ptr.hpp>
#define RTX_SHARED_POINTER(type) typedef boost::shared_ptr<type> sharedPointer

#define RTX_MAX_CHAR_STRING 256
#define RTX_MAX(x,y) x>y?x:y
#define RTX_MIN(x,y) x<y?x:y

// silly macro for string comparison
#include <boost/algorithm/string/predicate.hpp>
#define RTX_STRINGS_ARE_EQUAL(x,y) (boost::algorithm::iequals(x,y))
#define RTX_STRINGS_ARE_EQUAL_CS(x,y) (x.compare(y)==0)

#endif
