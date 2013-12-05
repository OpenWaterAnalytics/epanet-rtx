//
//  timeseries_demo.cpp
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  


#include <iostream>
#include <boost/foreach.hpp>

#include "Point.h"
#include "TimeSeries.h"
#include "Resampler.h"
#include "MovingAverage.h"
#include "FirstDerivative.h"
#include "SineTimeSeries.h"
#include "ConstantTimeSeries.h"
#include "AggregatorTimeSeries.h"
#include "CsvPointRecord.h"
#include "GainTimeSeries.h"

#include "BufferPointRecord.h"
#include "MysqlPointRecord.h"

//#include "SqliteProjectFile.h"

#include "SqlitePointRecord.h"

using namespace RTX;
using namespace std;

void printPoints(vector<Point> pointVector);

int main(int argc, const char * argv[])
{
  
  vector<Point> range;
  
  range.push_back(Point(101,1.));
  range.push_back(Point(102,1.));
  range.push_back(Point(103,1.));
  range.push_back(Point(104,1.));
  range.push_back(Point(105,1.));
  
  range.push_back(Point(106,1.));
  range.push_back(Point(107,1.));
  range.push_back(Point(108,1.));
  range.push_back(Point(109,1.));
  range.push_back(Point(110,1.));
  
  
  
  
  TimeSeries::sharedPointer ts(new TimeSeries());
  
  ts->setName("blah");
  
  {
    SqlitePointRecord::sharedPointer pr( new SqlitePointRecord() );
    pr->setPath("tempdb.sqlite");
    pr->dbConnect();
    
    ts->setRecord(pr);
    ts->insert(Point(100,1.));
    ts->insertPoints(range);
  }
  
  
  
  TimeSeries::sharedPointer ts2( new TimeSeries() );
  ts2->setName("blah");

  {
    SqlitePointRecord::sharedPointer pr( new SqlitePointRecord() );
    pr->setPath("tempdb.sqlite");
    pr->dbConnect();
    
    ts2->setRecord(pr);
    vector<Point> others = ts2->points(101, 110);
    
    printPoints(others);
  }
  
  
  
  
  
  
  /*
  ProjectFile::sharedPointer project(new SqliteProjectFile);
  
  project->loadProjectFile("testfile.rtx");
  */
  
  return 0;
}


void printPoints(vector<Point> pointVector) {
  BOOST_FOREACH(Point thePoint, pointVector) {
    cout << thePoint << endl;
  }
}

