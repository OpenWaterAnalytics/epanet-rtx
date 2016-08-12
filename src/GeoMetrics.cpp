#include "GeoMetrics.h"

extern "C" {
#include <geohash.h>
}

using namespace std;
using namespace RTX;


string GeoMetrics::geoHashWithJunction(Junction::_sp j, unsigned short int len) {
  const char* hashChar = geohash_encode(j->coordinates().second, j->coordinates().first, (int)len);
  const string hash(hashChar);
  return hash;
}


void GeoMetrics::_recalculateGrid() {
  
  _grid.clear();
  
  for(auto j : _model->junctions()) {
    const string hash = GeoMetrics::geoHashWithJunction(j, _gridSize);
    if (_grid.find(hash) == _grid.end()) {
      _grid[hash] = vector<Junction::_sp>();
    }
    _grid[hash].push_back(j);
  }
  
}




void GeoMetrics::poll() {
  
  for(const auto i : _grid) {
    const string hash(i.first);
    vector<Junction::_sp> junctions = i.second;
    
  }
  
}

