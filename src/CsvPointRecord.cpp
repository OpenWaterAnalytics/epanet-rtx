#include "CsvPointRecord.h"

#include <boost/foreach.hpp>
#include <boost/regex.hpp>

using namespace RTX;
using namespace std;
using namespace boost::filesystem;


CsvPointRecord::CsvPointRecord() {
  _isReadonly = false;
  _isConnected = false;
}

void CsvPointRecord::connect() throw(RtxException) {
  
  // expected format: "dir=path/to/folder;readonly=[yes/no]"
  std::string tokenizedString = this->connectionString();
  if (RTX_STRINGS_ARE_EQUAL(tokenizedString, "")) {
    return;
  }
  
  // de-tokenize
  std::map<std::string, std::string> kvPairs;
  {
    boost::regex kvReg("([^=]+)=([^;]+);?"); // key - value pair
    boost::sregex_iterator it(tokenizedString.begin(), tokenizedString.end(), kvReg), end;
    for ( ; it != end; ++it) {
      kvPairs[(*it)[1]] = (*it)[2];
    }
    
    // if any of the keys are missing, just return.
    if (kvPairs.find("dir") == kvPairs.end()) {
      cerr << "CSV directory not specified!" << endl;
      return;
    }
    if (kvPairs.find("readonly") != kvPairs.end()) {
      const string& readonly = kvPairs["readonly"];
      if (RTX_STRINGS_ARE_EQUAL(readonly, "yes")) {
        _isReadonly = true;
      }
    }
  }
  const string& dir = kvPairs["dir"];
  
  // try to access the directory
  _path = path(dir);
  
  bool dirOk = (exists(_path) && is_directory(_path));
  if (exists(_path) && !is_directory(_path)) {
    cerr << "CSV path specified exists, but it is not a directory! " << endl;
    return;
  }
  
  if (!dirOk && !_isReadonly) {
    // there's no directory here, so it's up to me to create it.
    if (!create_directory(_path)) {
      cerr << "CSV : could not create directory. check permissions?" << endl;
      return;
    }
    else {
      dirOk = true;
    }
  }
  
  if (dirOk) {
    // load in any data that may be in that dir
    
    loadDataFromCsvDir(_path);
    _isConnected = true;
  }
  
  
}

void CsvPointRecord::loadDataFromCsvDir(boost::filesystem::path dirPath) {
  
  // quick check
  bool dirOk = (exists(_path) && is_directory(_path));
  if (!dirOk) {
    cerr << "could not load data: directory not ok" << endl;
    return;
  }
  
  // get the list of files
  vector<path> files;
  copy(directory_iterator(dirPath), directory_iterator(), back_inserter(files));
  sort(files.begin(), files.end());
  
  // debug
  cout << "CSV Point Record found " << files.size() << " files:" << endl;
  BOOST_FOREACH(path p, files) {
    cout << p.filename() << endl;
  }
  
  
  
  BOOST_FOREACH(path p, files) {
    
    // get the file and parse it into my buffer.
    
    
  }
  
  
  
  
  
}

bool CsvPointRecord::isConnected() {
  return _isConnected;
}

std::string CsvPointRecord::registerAndGetIdentifier(std::string recordName) {
  
}

/* using base impl
std::vector<std::string> CsvPointRecord::identifiers() {
  
}

PointRecord::time_pair_t CsvPointRecord::range(const std::string& id) {
  
}
*/

std::ostream& CsvPointRecord::toStream(std::ostream &stream) {
  stream << "CSV Record: " << _path.filename() << endl;
  return stream;
}


std::vector<Point> CsvPointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  vector<Point> empty;
  return empty;
}

Point CsvPointRecord::selectNext(const std::string& id, time_t time) {
  return Point();
}

Point CsvPointRecord::selectPrevious(const std::string& id, time_t time) {
  return Point();
}


void CsvPointRecord::insertSingle(const std::string& id, Point point) {
  if (_isReadonly) {
    return;
  }
  else {
    
  }
}

void CsvPointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  if (_isReadonly) {
    return;
  }
  else {
    
  }
}

void CsvPointRecord::removeRecord(const std::string& id) {
  if (_isReadonly) {
    return;
  }
  else {
    
  }
}

void CsvPointRecord::truncate() {
  if (_isReadonly) {
    return;
  }
  else {
    
  }
}






