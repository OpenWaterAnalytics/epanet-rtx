//
//  ForecastTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 8/26/14.
//
//

#include "ForecastTimeSeries.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <sstream>

using namespace RTX;
using namespace std;


ForecastTimeSeries::ForecastTimeSeries() : _order(0,0,0) {
  
  _fitTime = 0;
  _refitClock = Clock::_sp( new Clock(60) );
  
  
  _python = PythonInterpreter::sharedInterpreter();
  
  _python->exec("from datetime import datetime");
  _python->exec("from datetime import timedelta");
  _python->exec("from scipy import stats");
  _python->exec("import pandas as pd");
  _python->exec("import statsmodels.api as sm");
  
}



ForecastTimeSeries::ArimaOrder ForecastTimeSeries::order() {
  return _order;
}

void ForecastTimeSeries::setOrder(ForecastTimeSeries::ArimaOrder order) {
  _order = order;
}

void ForecastTimeSeries::setOrder(int ar, int i, int ma) {
  ArimaOrder theOrder(ar,i,ma);
  this->setOrder(theOrder);
}



vector<Point> ForecastTimeSeries::filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime) {
  
  // these points you are requesting...
  // how does that compare to my last-fit model?
  // there are three possibilities:
  // 1.  fully before the next re-fit
  // 2.  fully after the next re-fit
  // 3.  straddle these two regimes.
  
  bool needsRefit = false;
  
  time_t nextFit = _refitClock->timeAfter(_fitTime);
  
  if (toTime <= nextFit) {
    // requested points are fully before the next required re-fit
    // i can use the currently fit model to estimate.
  }
  else if (nextFit <= fromTime) {
    // requested points are fully after the next reqired re-fit
    // let's re-fit the model.
    
    // this->reFitModelFromTime(nextFit);
    
    
  }
  else if ( fromTime < nextFit && nextFit < toTime ) {
    // two regimes.
    // get the points in the time range valid for this model fit.
    vector<Point> priorModelPoints = this->filteredPoints(sourceTs, fromTime, nextFit);
    vector<Point> nextModelPoints = this->filteredPoints(sourceTs, nextFit, toTime);
    
    vector<Point> combined; // == combine them
    BOOST_FOREACH(const Point& p, priorModelPoints) {
      combined.push_back(p);
    }
    BOOST_FOREACH(const Point& p, nextModelPoints) {
      combined.push_back(p);
    }
    
    return combined;
  }
  
  
  // use whatever model params we've figured out, and do the estimation.
  
  
  vector<Point> sourcePoints = sourceTs->points(fromTime, toTime);
  
  stringstream pandasName;
  pandasName << "ts__pd_ts";
  
  this->pointsToPandas(sourcePoints, pandasName.str());
  
  
  vector<Point> resultPoints = this->pointsFromPandas(pandasName.str());
  
  return resultPoints;
  
}



vector<Point> ForecastTimeSeries::pointsFromPandas(std::string varName) {
  vector<Point> points;
  
  stringstream evalStr;
  evalStr << "len(" << varName << ".index)";
  int nPts = _python->intValueFromEval(evalStr.str());
  
  
  
  for (int i=0; i < nPts; ++i) {
    
    stringstream tStr, vStr;
    tStr << "long((" << varName << ".index[" << i << "] - datetime(1970, 1, 1)).total_seconds())";
    vStr << varName << ".values[" << i << ",0]";
    
    time_t time = (time_t)_python->longIntValueFromEval(tStr.str());
    double value = _python->doubleValueFromEval(vStr.str());
    
    if (time == 0) {
      continue;
    }
    
    points.push_back(Point(time,value));
  }
  
  return points;
}

void ForecastTimeSeries::pointsToPandas(std::vector<Point> points, std::string pandasVarName) {
  
  
  
  
  stringstream delVal, delTime;
  delVal << pandasVarName << "_values = None";
  delTime << pandasVarName << "_times = None";
  _python->exec(delVal.str());
  _python->exec(delTime.str());
  
  
  stringstream newVal, newTm;
  
  newVal << pandasVarName << "_values = list()";
  newTm << pandasVarName << "_times = list()";
  _python->exec(newVal.str());
  _python->exec(newTm.str());
  
  
  BOOST_FOREACH(const Point& p, points) {
    
    stringstream valStr, tmStr;
    valStr << pandasVarName << "_values.append(" << p.value << ")";
    tmStr  << pandasVarName << "_times.append(" << p.time << ")";
    
    _python->exec(valStr.str());
    _python->exec(tmStr.str());
    
  }
  
  stringstream tsCreate, tsIndex;
  tsCreate << pandasVarName << " = pd.DataFrame.from_dict({\"values\": " << pandasVarName << "_values})";
  tsIndex << pandasVarName << ".index = pd.to_datetime(" << pandasVarName << "_times, unit=\'s\')";
  
  _python->exec(tsCreate.str());
  _python->exec(tsIndex.str());
  
}





bool ForecastTimeSeries::pointsRegular(vector<Point> points) {
  
  size_t nPoints = points.size();
  if (nPoints <= 2) {
    return true;
  }
  
  time_t dt = 0;
  dt = points.at(1).time - points.at(0).time;
  
  vector<Point>::const_iterator p_it = points.begin();
  
  Point p1 = *p_it;
  ++p_it;
  
  while (p_it != points.end()) {
    
    if (p_it->time - p1.time != dt) {
      return false;
    }
    
    p1 = *p_it;
    ++p_it;
    
  }
  
  return true;
  
}







