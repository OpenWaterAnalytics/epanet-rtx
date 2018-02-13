//
//  AutoRunner.hpp
//  LINK-service
//
//  Created by sam hatchett on 2/12/18.
//

#ifndef AutoRunner_hpp
#define AutoRunner_hpp

#include <stdio.h>
#include <atomic>
#include <future>

#include "TimeSeries.h"

#define RTX_AUTORUNNER_LOGLEVEL_ERROR   0
#define RTX_AUTORUNNER_LOGLEVEL_WARN    1
#define RTX_AUTORUNNER_LOGLEVEL_INFO    2
#define RTX_AUTORUNNER_LOGLEVEL_VERBOSE 3


namespace RTX {
  class AutoRunner {
  public:
    AutoRunner();
    void setSeries(std::vector<TimeSeries::_sp> series);
    void setParams(bool smartQueries, int maxWindowSeconds, int frequencySeconds, int throttleSeconds);
    void run(time_t since);
    void cancel();
    bool isRunning();
    void setLogging(std::function<void(std::string)> fn, int level);
    
    
  private:
    std::future<void> _task;
    std::atomic_bool _cancellation_token, _is_running_token;
    std::function<void(std::string)> _logFn;
    int _logLevel;
    bool _smart;
    int _window;
    int _freq;
    int _throttle;
    
    class TsEntry {
    public:
      TimeSeries::_sp series;
      time_t lastGood;
    };
    std::vector<TsEntry> _series;
    
    void _runLoop(std::atomic_bool& cancel);
    void _log(std::string msg, int msgLevel);
    
  };
}


#endif /* AutoRunner_hpp */
