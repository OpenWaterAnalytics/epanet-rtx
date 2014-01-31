#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <boost/foreach.hpp>

#include "CsvPointRecord.h"

using namespace RTX;
using namespace std;
using namespace boost::filesystem;

CsvPointRecord::CsvPointRecord() {
  _isReadonly = false;
}

CsvPointRecord::~CsvPointRecord() {
  // upon destruction, save all data back into csv files.
  if (_isReadonly) {
    cout << "warning: csv record " << _path.filename().stem().string() << " may be losing changes (readonly)" << endl;
    return;
  }
  saveDataToCsvDir(_path);
}


void CsvPointRecord::setPath(const std::string& pathStr) {
  // try to access the directory
  _path = path(pathStr);
  
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
  }
}

void CsvPointRecord::setReadOnly(bool readOnly) {
  _isReadonly = readOnly;
}

bool CsvPointRecord::isReadOnly() {
  return _isReadonly;
}

void CsvPointRecord::loadDataFromCsvDir(boost::filesystem::path dirPath) {
  
  // quick check
  bool dirOk = (exists(_path) && is_directory(_path));
  if (!dirOk) {
    cerr << "could not load data from [" << _path << "] :: directory not ok" << endl;
    return;
  }
  
  // get the list of files
  vector<path> files;
  copy(directory_iterator(dirPath), directory_iterator(), back_inserter(files));
  sort(files.begin(), files.end());
  
  BOOST_FOREACH(path p, files) {
    
    // check the file name
    const string ext( p.filename().extension().string() );
    if ( ! RTX_STRINGS_ARE_EQUAL(ext, ".csv") ) {
      //cerr << "warning: ignoring file " << p.filename() << " because it's not a csv" << endl;
      continue;
    }
    
    const string tsName( p.stem().string() );
    vector<Point> pointContents;
    ifstream csvFileStream(p.c_str());
    string line;
    time_t time;
    double value;
    string comma;
    
    // get the file and parse it into my buffer.
    if (csvFileStream.is_open()) {
      while (csvFileStream.good()) {
        getline(csvFileStream, line);
        stringstream ss(line);
        // comment line -- todo - add units?
        if (ss.peek() == '#') {
          ss.ignore(); // skip comment character
          string comment;
          ss >> comment;
          //cout << "Comment: " << comment << endl;
          continue;
        }
        ss >> time;
        if (ss.peek() == ',') {
          ss.ignore();
        }
        ss >> value;
        if (ss.fail()) {
          // end of parse
          continue;
        }
        Point newPoint(time, value);
        //cout << newPoint << endl;
        pointContents.push_back(newPoint);
      } // EOF
      
      csvFileStream.close();
      this->registerAndGetIdentifier(tsName, Units());
      RTX_CSVPR_SUPER::addPoints(tsName, pointContents); // cache them in the superclass
    } // file opened
    else {
      cerr << "File stream not good: " << p.filename() << endl;
    }
  }
}

void CsvPointRecord::saveDataToCsvDir(boost::filesystem::path dirPath) {
  if (isReadOnly()) {
    cerr << "cannot save to csv: point record is read-only" << endl;
    return;
  }
  
  vector<string> ids = this->identifiers();
  BOOST_FOREACH(string id, ids) {
    // the timeseries name becomes the filename.
    path newCsv = _path;
    newCsv /= id;
    newCsv += ".csv";
    
    ofstream csvOfStream(newCsv.c_str());
    if (csvOfStream.good()) {
      time_pair_t range = this->range(id);
      vector<Point> points = this->pointsInRange(id, range.first, range.second);
      BOOST_FOREACH(Point p, points) {
        csvOfStream << p.time << ", " << setprecision(12) << p.value << endl;
      }
    } // ostream good
    csvOfStream.close();
  } // foreach id
  
}


std::ostream& CsvPointRecord::toStream(std::ostream &stream) {
  stream << "CSV Record: " << _path.filename() << endl;
  return stream;
}

