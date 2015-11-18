#include <stdlib.h>
#include <cstdio>
#include "TimeSeriesDuplicator.h"
#include "SqliteProjectFile.h"
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <signal.h>

using namespace std;
using namespace RTX;

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
  
  // the requirements here are constrained.
  // you must specify the path for a SQLITE-formatted project file,
  // with a set of timeseries referencing a Source record, and a second
  // record with no timeseries associated that will be the destination.
  // also, you must specify the fetch window and frequency in the meta table
  // of the config db. Keys are "duplicatorFetchFrequency" and "duplicatorFetchWindow"
  
  // if the argument is not supplied, then use a default location
  string projectPath;
  
  if (argc == 2) {
    const char *configPathChar = argv[1];
    projectPath = string(configPathChar);
  }
  else if (argc == 1) {
    projectPath = "/opt/rtx/rtxduplicator.rtx";
  }
  else {
    cerr << "usage: " << argv[0] << " [/path/to/config.rtx]" << endl;
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
  
  
  while (true) {
    _duplicator->run(fetchWindow, fetchFrequency);
    if (!_duplicator->_shouldRun) {
      break;
    }
    std::cerr << "Duplication process quit for some reason. Restarting in 30s" << std::endl << std::flush;
    boost::this_thread::sleep_for(boost::chrono::seconds(30));
  }
  
  
  
  
  
  return 0;
}


