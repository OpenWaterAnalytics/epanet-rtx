//
//  ForecastTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 8/26/14.
//
//

#ifndef __epanet_rtx__ForecastTimeSeries__
#define __epanet_rtx__ForecastTimeSeries__

#include <iostream>

#include "ModularTimeSeries.h"
#include "PythonInterpreter.h"

namespace RTX {
  
  
  /*!
   \class ForecastTimeSeries
   \brief An arima - based time series predictor/forecaster.
   
   */
  
  /*!
   \fn virtual Point TimeSeries::point(time_t time)
   \brief Get a Point at a specific time.
   \param time The requested time.
   \return The requested Point, or the Point just prior to the passed time value.
   \sa Point
   */
  /*!
   \fn virtual std::vector<Point> TimeSeries::points(time_t start, time_t end)
   \brief Get a vector of Points within a specific time range.
   \param start The beginning of the requested time range.
   \param end The end of the requested time range.
   \return The requested Points (as a vector)
   
   The base class provides some brute-force logic to retrieve points, by calling Point() repeatedly. For more efficient access, you may wish to override this method.
   
   \sa Point
   */
  
  
  class ForecastTimeSeries : public ModularTimeSeries {
  public:
    
    class ArimaOrder {
    public:
      ArimaOrder(int autoregressive, int integratred, int movingaverage) : ar(autoregressive), i(integratred), ma(movingaverage) {};
      int ar;
      int i;
      int ma;
    };
    
    
    RTX_SHARED_POINTER(ForecastTimeSeries);
    ForecastTimeSeries();
    
    Clock::sharedPointer fitWindow();
    void setFitWindow(Clock::sharedPointer clock);
    
    Clock::sharedPointer refitClock();
    void setRefitClock(Clock::sharedPointer clock);
    
    
    ArimaOrder order();
    void setOrder(ArimaOrder order);
    void setOrder(int ar, int i, int ma);
    
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  private:
    time_t _fitTime; // last time the model was fit
    ArimaOrder _order; // model order
    Clock::sharedPointer _fitWindow, _refitClock;
    PythonInterpreter::sharedPointer _python;
    
    void reFitModelFromTime(time_t refitTime);
    
    bool pointsRegular(std::vector<Point> points); // convenience checking
    
    std::vector<Point> pointsFromPandas(std::string varName);
    void pointsToPandas(std::vector<Point> points, std::string pandasVarName);
    
  };
}


#endif /* defined(__epanet_rtx__ForecastTimeSeries__) */
