#include <iostream>
#include <time.h>
#include <boost/foreach.hpp>
#include <iomanip>

#include "EpanetModel.h"
#include "SqliteProjectFile.h"

using namespace std;
using namespace RTX;


int main (int argc, const char * argv[])
{
  int intQual = 332;
  string tStr = "2014-05-06 12:32:02";
  double v = 223.4455656632;
  
  std::bitset<16> bits(intQual);
  cout << "#" << setw(20) << tStr << " | " << setw(10) << v << " | " << "Q: " << setw(6) << intQual << " | " << setw(16) << bits << endl;
  
  cout << endl;
  
  return 0;
  
  
  {
    
    ProjectFile::_sp project( new SqliteProjectFile);
    project->loadProjectFile("/Users/sam/Desktop/milford.rtx");
    
    
  }
  cout << "done" << endl;
  

  
  return 0;
}