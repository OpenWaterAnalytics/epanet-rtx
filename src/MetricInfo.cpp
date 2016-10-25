#include "MetricInfo.h"

#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace std;
using namespace RTX;

// METRIC INFO UTILITY CLASS
MetricInfo::MetricInfo(const string& name) {
  size_t firstComma = name.find(",");
  // measure name is everything up to the first comma, even if that's everything
  this->measurement = name.substr(0,firstComma);
  if (firstComma != string::npos) {
    // a comma was found. therefore treat the name as tokenized
    string keysValuesStr = name.substr(firstComma+1);
    regex kvReg("([^=]+)=([^,]+),?"); // key - value pair
    auto kv_begin = sregex_iterator(keysValuesStr.begin(), keysValuesStr.end(), kvReg);
    auto kv_end = sregex_iterator();
    for (auto i = kv_begin; i != kv_end; ++i) {
      this->tags[(*i)[1]] = (*i)[2];
    }
  }
}

const string MetricInfo::name() {
  stringstream ss;
  ss << this->measurement;
  for (auto p : this->tags) {
    ss << "," << p.first << "=" << p.second;
  }
  const string name = ss.str();
  return name;
}

string MetricInfo::properId(const std::string &id) {
  return MetricInfo(id).name();
}
