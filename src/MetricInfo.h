#ifndef MetricInfo_h
#define MetricInfo_h

#include <stdio.h>
#include <string>
#include <map>

namespace RTX {
  class MetricInfo {
  public:
    MetricInfo(const std::string& name); // ctor from string name
    const std::string name();
    static std::string properId(const std::string& id);
    std::map<std::string, std::string> tags;
    std::string measurement;
  };
}



#endif /* MetricInfo_h */
