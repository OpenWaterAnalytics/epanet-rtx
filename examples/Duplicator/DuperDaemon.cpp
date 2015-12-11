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
volatile bool _duplicator_should_run;

void(^logMsgCallback)(const char*) = ^(const char* msg) {
  string myLine(msg);
  size_t loc = myLine.find("\n");
  if (loc == string::npos) {
    myLine += "\n";
  }
  const char *logmsg = myLine.c_str();
  fprintf(stdout, "%s", logmsg);
};

void handleInterrupt(int sig) {
  if (!_duplicator) {
    return;
  }
  logMsgCallback("notifying duplication process to stop");
  _duplicator->stop();
  _duplicator_should_run = false;
}

int main (int argc, const char * argv[])
{
  _duplicator_should_run = true;
  signal(SIGINT, handleInterrupt);
  signal(SIGTERM, handleInterrupt);
  
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help", "produce help message")
  ("path", po::value<string>()->default_value("/opt/rtx/rtxduplicator.rtx"), "config database path. default /opt/rtx/rtxduplicator.rtx")
  ("catchup", po::value<int>(), "start [n] days ago, and catchup in 1-day increments")
  ("ratelimit", po::value<int>(), "minimum number of seconds between duplication chunks (default 0, no limit)")
  ("odbcinst", po::value<string>(), "path to odbcinst.ini file declaring location of any relevant drivers referenced in your config")
  ("nometrics","do not store metrics in destination db.");
  ;
  po::variables_map vars;
  po::store(po::parse_command_line(argc, argv, desc), vars);
  po::notify(vars);
  
  if (vars.count("help")) {
    cout << desc << "\n";
    return 1;
  }
  
  if (vars.count("odbcinst")) {
    int err = 0;
    string path = vars["odbcinst"].as<string>();
    err = setenv("ODBCINSTINI", path.c_str(), 1);
    if (err) {
      logMsgCallback("ERROR: could not set ENV var ODBCINSTINI");
    }
  }
  
  
  // the requirements here are constrained.
  // you must specify the path for a SQLITE-formatted project file,
  // with a set of timeseries referencing a Source record, and a second
  // record with no timeseries associated that will be the destination.
  // also, you must specify the fetch window and frequency in the meta table
  // of the config db. Keys are "duplicatorFetchFrequency" and "duplicatorFetchWindow"
  
  // if the argument is not supplied, then use a default location
  string projectPath = vars["path"].as<string>();
  
  
  _duplicator.reset(new TimeSeriesDuplicator);
  _duplicator->setLoggingFunction(logMsgCallback);
  
  SqliteProjectFile::_sp project(new SqliteProjectFile());
  
  bool checkForConfig = true;
  while (checkForConfig && _duplicator_should_run) {
    if(project->loadProjectFile(projectPath)) {
      checkForConfig = false;
    }
    else {
      std::cerr << "Could not load config. Waiting 15s" << std::endl << std::flush;
      boost::this_thread::sleep_for(boost::chrono::seconds(15));
    }
  }
  
  if (!_duplicator->_shouldRun) {
    return 0;
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
  
  if (RTX_STRINGS_ARE_EQUAL(freqStr, "") || RTX_STRINGS_ARE_EQUAL(winStr, "")) {
    logMsgCallback("Cannot use config: meta keys \"duplicatorFetchFrequency\" and \"duplicatorFetchWindow\" must be set!");
    return 1;
  }
  
  time_t fetchWindow = boost::lexical_cast<time_t>(winStr);
  time_t fetchFrequency = boost::lexical_cast<time_t>(freqStr);
  
  stringstream ss;
  ss << "Starting duplication service from " << sourceRecord->name() << " to " << _duplicator->destinationRecord()->name() << " for " << to_string(project->timeSeries().size()) << " time series" << endl;
  logMsgCallback(ss.str().c_str());
  
  bool catchup = vars.count("catchup") > 0;
  
  while (true) {
    try {
      if (catchup) {
        catchup = false;
        int nDays = vars["catchup"].as<int>();
        time_t limit = 0;
        if (vars.count("ratelimit")) {
          limit = (time_t)(vars["ratelimit"].as<int>());
        }
        logMsgCallback("=== RUNNING RETROSPECTIVE DUPLICATION SERVICE ===");
        _duplicator->runRetrospective(time(NULL) - (60*60*24*nDays), (60*60*24), limit); // catch up and stop.
      }
      if (!_duplicator->_shouldRun) {
        break;
      }
      logMsgCallback("=== RUNNING REALTIME DUPLICATION SERVICE ===");
      _duplicator->run(fetchWindow, fetchFrequency);
      if (!_duplicator->_shouldRun) {
        break;
      }
    } catch (std::string& err) {
      logMsgCallback("Duplication Exception:");
      logMsgCallback(err.c_str());
    } catch (...) {
      logMsgCallback("Unknown Duplication Exception.");
    }
    logMsgCallback("Duplication process quit for some reason. Restarting in 30s");
    boost::this_thread::sleep_for(boost::chrono::seconds(30));
  }
  
  return 0;
}
