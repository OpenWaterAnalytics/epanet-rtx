#include <iostream>
#include <time.h>
#include <boost/foreach.hpp>

#include "EpanetModel.h"
#include "SqliteProjectFile.h"

using namespace std;
using namespace RTX;


int main (int argc, const char * argv[])
{

  // try unit serialize/deserialize
  
  Units u = RTX_PSI;
  
  string uStr = u.unitString();
  cout << uStr << endl;
  
  Units u2 = Units::unitOfType(uStr);
  cout << "got back units: " << u2 << endl;
  
  cout << endl;
  
  Units u3 = u * RTX_GALLON;
  string u3str = u3.unitString();
  cout << "strange units string: " << u3str << endl;
  
  Units u4 = Units::unitOfType(u3str);
  cout << "got back units: " << u4 << endl;
  
  return 0;
  
  
  {
    
    ProjectFile::sharedPointer project( new SqliteProjectFile);
    project->loadProjectFile("/Users/sam/Desktop/milford.rtx");
    
    
  }
  cout << "done" << endl;
  

  
  return 0;
}