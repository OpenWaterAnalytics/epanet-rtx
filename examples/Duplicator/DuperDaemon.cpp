#include <stdlib.h>
#include <cstdio>
#include "TimeSeriesDuplicator.h"
#include "SqliteProjectFile.h"
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <signal.h>

using namespace std;
using namespace RTX;
namespace po = boost::program_options;

TimeSeriesDuplicator::_sp _duplicator;
void handleInterrupt(int sig);
void handleInterrupt(int sig) {
  if (!_duplicator) {
    return;
  }
  _duplicator->stop();
}

void(^logMsgCallback)(const char*) = ^(const char* msg) {
  string myLine(msg);
  size_t loc = myLine.find("\n");
  if (loc == string::npos) {
    myLine += "\n";
  }
  const char *logmsg = myLine.c_str();
  fprintf(stdout, "%s", logmsg);
};

int main (int argc, const char * argv[])
{
  
  signal(SIGINT, handleInterrupt);
  
  
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help", "produce help message")
  ("path", po::value<string>(), "config database path. default /opt/rtx/rtxduplicator.rtx")
  ("catchup", po::value<int>(), "start [n] days ago, and catchup in 1-day increments")
  ("ratelimit", po::value<int>(), "minimum number of seconds between duplication chunks (default 0, no limit)")
  ;
  po::variables_map vars;
  po::store(po::parse_command_line(argc, argv, desc), vars);
  po::notify(vars);
  
  if (vars.count("help")) {
    cout << desc << "\n";
    return 1;
  }
  
  // the requirements here are constrained.
  // you must specify the path for a SQLITE-formatted project file,
  // with a set of timeseries referencing a Source record, and a second
  // record with no timeseries associated that will be the destination.
  // also, you must specify the fetch window and frequency in the meta table
  // of the config db. Keys are "duplicatorFetchFrequency" and "duplicatorFetchWindow"
  
  // if the argument is not supplied, then use a default location
  string projectPath;
  if (vars.count("path")) {
    projectPath = vars["path"].as<string>();
  } else {
    projectPath = "/opt/rtx/rtxduplicator.rtx";
  }
  
  
  
  _duplicator.reset(new TimeSeriesDuplicator);
  _duplicator->setLoggingFunction(logMsgCallback);
  
  SqliteProjectFile::_sp project(new SqliteProjectFile());
  
  bool checkForConfig = true;
  while (checkForConfig) {
    if(project->loadProjectFile(projectPath)) {
      checkForConfig = false;
    }
    else {
      std::cerr << "Could not load config. Waiting 15s" << std::endl << std::flush;
      boost::this_thread::sleep_for(boost::chrono::seconds(15));
    }
  }
  
  _duplicator->setSeries(project->timeSeries()); /// these are source series.
  
  // get a random time series and get its record.
  TimeSeries::_sp ts1 = project->timeSeries().front();
  PointRecord::_sp sourceRecord = ts1->record();
  
  PointRecord::_sp destinationRecord;
  
  BOOST_FOREACH(PointRecord::_sp r, project->records()) {
    if (r != sourceRecord) {
      destinationRecord = r;
    }
    r->identifiersAndUnits(); // force dbConnect
  }
  
  
  if (!destinationRecord) {
    cerr << "No destination record specified" << endl;
    return 100;
  }
  
  _duplicator->setDestinationRecord(destinationRecord);
  
  string freqStr = project->metaValueForKey(string("duplicatorFetchFrequency"));
  string winStr = project->metaValueForKey(string("duplicatorFetchWindow"));
  
  time_t fetchWindow = boost::lexical_cast<time_t>(winStr);
  time_t fetchFrequency = boost::lexical_cast<time_t>(freqStr);
  
  cout << "Starting duplication service from " << sourceRecord->name() << " to " << _duplicator->destinationRecord()->name() << " for " << to_string(project->timeSeries().size()) << " time series" << endl;
  
  bool catchup = vars.count("catchup") > 0;
  
  while (true) {
    
    if (catchup) {
      catchup = false;
      int nDays = vars["catchup"].as<int>();
      time_t limit = 0;
      if (vars.count("ratelimit")) {
        limit = vars["ratelimit"].as<time_t>();
      }
      _duplicator->runRetrospective(time(NULL) - (60*60*24*nDays), (60*60*24), limit); // catch up and stop.
    }
    if (!_duplicator->_shouldRun) {
      break;
    }
    
    
    _duplicator->run(fetchWindow, fetchFrequency);
    if (!_duplicator->_shouldRun) {
      break;
    }
    std::cerr << "Duplication process quit for some reason. Restarting in 30s" << std::endl << std::flush;
    boost::this_thread::sleep_for(boost::chrono::seconds(30));
  }
  
  
  
  
  
  return 0;
}


