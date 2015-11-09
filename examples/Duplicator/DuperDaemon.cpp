#include <stdlib.h>
#include "TimeSeriesDuplicator.h"
#include "SqliteProjectFile.h"
#include <boost/lexical_cast.hpp>
#include <signal.h>

using namespace std;
using namespace RTX;

TimeSeriesDuplicator::_sp _duplicator;

void handleInterrupt(int sig) {
  if (!_duplicator) {
    return;
  }
  _duplicator->stop();
}

void(^logMsgCallback)(const char*) = ^(const char* msg) {
  cout << msg;
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
    projectPath = "/usr/local/DuperDaemon/config.rtx";
  }
  else {
    cerr << "usage: " << argv[0] << " [/path/to/config.rtx]" << endl;
  }
  
  _duplicator.reset(new TimeSeriesDuplicator);
  _duplicator->setLoggingFunction(logMsgCallback);
  
  SqliteProjectFile::_sp project(new SqliteProjectFile());
  if (!project->loadProjectFile(projectPath)) {
    cerr << "could not find project file" << endl;
    return 100;
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
  
  _duplicator->run(fetchWindow, fetchFrequency);
  
  
  
  return 0;
}


