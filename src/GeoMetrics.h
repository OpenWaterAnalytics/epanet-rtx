#ifndef GeoMetrics_h
#define GeoMetrics_h

#include <stdio.h>
#include <map>
#include <string>
#include <vector>

#include "TimeSeries.h"
#include "Model.h"
#include "Junction.h"
#include "PointRecord.h"

namespace RTX {
  class GeoMetrics {
  public:
    typedef std::shared_ptr<GeoMetrics> _sp;
    GeoMetrics(Model::_sp model, PointRecord::_sp record) : _model(model), _record(record) {};
    
    static std::string geoHashWithJunction(Junction::_sp j, unsigned short int len = 12);
    
    void setGridSize(int gridsize);
    int gridSize();
    std::map<std::string, std::vector<Junction::_sp> > grid();
    
    void poll();
    
    typedef struct {
      TimeSeries::_sp min, max, mean;
    } modelStatistic;
    
    typedef struct {
      modelStatistic pressure, quality, demand;
    } statCollection;
    
    
  private:
    void _recalculateGrid();
    std::map<std::string, std::vector<Junction::_sp> > _grid;
    std::map<std::string, statCollection> _statistics;
    unsigned short int _gridSize;
    Model::_sp _model;
    PointRecord::_sp _record;
    
  };
}

#endif /* GeoMetrics_hpp */
