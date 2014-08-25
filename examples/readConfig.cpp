#include <iostream>
#include <time.h>
#include <boost/foreach.hpp>

#include "EpanetModel.h"
#include "SqliteProjectFile.h"

using namespace std;
using namespace RTX;


int main (int argc, const char * argv[])
{

  
  {
    
    ProjectFile::sharedPointer project( new SqliteProjectFile);
    project->loadProjectFile("/Users/sam/Desktop/milford.rtx");
    
    
  }
  cout << "done" << endl;
  

  
  return 0;
}