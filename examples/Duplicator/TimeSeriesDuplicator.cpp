#include "TimeSeriesDuplicator.h"



#include <boost/interprocess/sync/scoped_lock.hpp>
using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

using namespace std;
using namespace RTX;


PointRecord::_sp TimeSeriesDuplicator::destinationRecord() {
  return _destinationRecord;
}



