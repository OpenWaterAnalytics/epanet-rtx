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

#include "TimeSeriesFilter.h"
#include "PythonInterpreter.h"

namespace RTX {
  
  
  /*!
   \class ForecastTimeSeries
   \brief An arima - based time series predictor/forecaster.
   
   */
  
  /*!
   \fn Clock::_sp ForecastTimeSeries::fitWindow();
   \brief The window that the ForecastTimeSeries uses to poll its source for the fit population.
   */
  /**
   \fn ForecastTimeSeries::SARIMAOrder ForecastTimeSeries::order();
   \brief Set the SARIMA orders that the ForecastTimeSeries will use.
   */
  /*!
   \fn Clock::_sp ForecastTimeSeries::refitClock();
   \brief The clock that ForecastTimeSeries will use to re-fit the model.
   */
  
  
  class ForecastTimeSeries : public TimeSeriesFilter {
  public:
    
    class SARIMAOrder {
    public:
      SARIMAOrder(int seasonal, int autoregressive, int integratred, int movingaverage) : s(seasonal), ar(autoregressive), i(integratred), ma(movingaverage) {};
      int s;
      int ar;
      int i;
      int ma;
    };
    
    
    RTX_SHARED_POINTER(ForecastTimeSeries);
    ForecastTimeSeries();
    
    Clock::_sp fitWindow();
    void setFitWindow(Clock::_sp clock);
    
    Clock::_sp refitClock();
    void setRefitClock(Clock::_sp clock);
    
    
    SARIMAOrder order();
    void setOrder(SARIMAOrder order);
    
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime);
    
  private:
    time_t _fitTime; // last time the model was fit
    ArimaOrder _order; // model order
    Clock::_sp _fitWindow, _refitClock;
    PythonInterpreter::_sp _python;
    
    void reFitModelFromTime(time_t refitTime);
    
    bool pointsRegular(std::vector<Point> points); // convenience checking
    
    std::vector<Point> pointsFromPandas(std::string varName);
    void pointsToPandas(std::vector<Point> points, std::string pandasVarName);
    
  };
}


#endif /* defined(__epanet_rtx__ForecastTimeSeries__) */
