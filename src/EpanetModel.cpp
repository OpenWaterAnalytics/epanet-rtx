//
//  EpanetModel.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iostream>
#include "EpanetModel.h"
#include "rtxMacros.h"
#include <CurveFunction.h>

#include <types.h>

#include <boost/filesystem.hpp>

using namespace RTX;
using namespace TSF;
using namespace std;

#define SMALL 0.001

EpanetModel::EpanetModel() : Model() {
  // nothing to do, right?
  _enOpened = false;
  _enModel = NULL;
//  EN_API_CHECK( EN_newModel(&_enModel), "EN_newModel");
}

EpanetModel::~EpanetModel() {
  this->closeEngine();
  if (_enOpened) {
    EN_API_CHECK( EN_close(_enModel), "EN_close");
  }
  //  EN_API_CHECK(EN_freeModel(_enModel), "EN_freeModel");
}

EpanetModel::EpanetModel(const EpanetModel& o) {
  
  cout << "copy ctor : EpanetModel" << endl;
  
  this->useEpanetFile(o._modelFile);
  this->createRtxWrappers();
  
}

EN_Project EpanetModel::epanetModelPointer() {
  return _enModel;
}

EpanetModel::EpanetModel(const std::string& filename) {
  try {
    this->useEpanetFile(filename);
    this->createRtxWrappers();
  }
  catch(const std::string& errStr) {
    std::cerr << "ERROR: ";
    throw RtxException("File Loading Error: " + errStr);
  }
}

#pragma mark - Loading

void EpanetModel::useEpanetModel(EN_Project model, string path) {
  Units volumeUnits(0);
  this->_enModel = model;
  _modelFile = path;
  
  // get units from epanet
  int flowUnitType = 0;
  bool isSI = false;
  
  int qualCode, traceNode;
  char chemName[56], chemUnits[56];
  EN_API_CHECK( EN_getqualinfo(_enModel, &qualCode, chemName, chemUnits, &traceNode), "EN_getqualinfo" );
  string chemUnitsStr(chemUnits);
  if (chemUnitsStr == "hrs") {
    chemUnitsStr = "hr";
  }
  //Units qualUnits = Units::unitOfType(chemUnitsStr);
  //this->setQualityUnits(qualUnits);
    
  EN_API_CHECK( EN_getflowunits(_enModel, &flowUnitType), "EN_getflowunits");
  switch (flowUnitType)
  {
    case EN_LPS:
    case EN_LPM:
    case EN_MLD:
    case EN_CMH:
    case EN_CMD:
      isSI = true;
      break;
    default:
      isSI = false;
  }
  switch (flowUnitType) {
    case EN_LPS:
      setFlowUnits(TSF_LITER_PER_SECOND);
      break;
    case EN_MLD:
      setFlowUnits(TSF_MILLION_LITER_PER_DAY);
      break;
    case EN_CMH:
      setFlowUnits(TSF_CUBIC_METER_PER_HOUR);
      break;
    case EN_CMD:
      setFlowUnits(TSF_CUBIC_METER_PER_DAY);
      break;
    case EN_GPM:
      setFlowUnits(TSF_GALLON_PER_MINUTE);
      break;
    case EN_MGD:
      setFlowUnits(TSF_MILLION_GALLON_PER_DAY);
      break;
    case EN_IMGD:
      setFlowUnits(TSF_IMPERIAL_MILLION_GALLON_PER_DAY);
      break;
    case EN_AFD:
      setFlowUnits(TSF_ACRE_FOOT_PER_DAY);
    default:
      break;
  }
  
  if (isSI) {
    setHeadUnits(TSF_METER);
    setPressureUnits(TSF_KILOPASCAL);
    volumeUnits = TSF_CUBIC_METER;
    
  }
  else {
    setHeadUnits(TSF_FOOT);
    setPressureUnits(TSF_PSI);
    volumeUnits = TSF_CUBIC_FOOT;
  }
  
  this->setVolumeUnits(volumeUnits);
  
  // what units are quality in? who knows!
  //this->setQualityUnits(TSF_MICROSIEMENS_PER_CM);
  //EN_API_CHECK(EN_setqualtype(_enModel, CHEM, (char*)"rtxConductivity", (char*)"us/cm", (char*)""), "EN_setqualtype");
    
  // get simulation parameters
  {
    long enTimeStep;
    EN_API_CHECK(EN_gettimeparam(_enModel, EN_HYDSTEP, &enTimeStep), "EN_gettimeparam EN_HYDSTEP");
    this->setHydraulicTimeStep((int)enTimeStep);

    long enQStep;
    EN_API_CHECK(EN_gettimeparam(_enModel, EN_QUALSTEP, &enQStep), "EN_gettimeparam EN_QUALSTEP");
    this->setQualityTimeStep((int)enQStep);
  }
  
  {
    long reportStep;
    EN_API_CHECK(EN_gettimeparam(_enModel, EN_REPORTSTEP, &reportStep), "EN_gettimeparam EN_REPORTSTEP");
    this->setReportTimeStep((int)reportStep);
  }
  
  int nodeCount, tankCount, linkCount;
  char enName[RTX_MAX_CHAR_STRING];
    
  EN_API_CHECK( EN_getcount(_enModel, EN_NODECOUNT, &nodeCount), "EN_getcount EN_NODECOUNT" );
  EN_API_CHECK( EN_getcount(_enModel, EN_TANKCOUNT, &tankCount), "EN_getcount EN_TANKCOUNT" );
  EN_API_CHECK( EN_getcount(_enModel, EN_LINKCOUNT, &linkCount), "EN_getcount EN_LINKCOUNT" );
  
  // store the original number of simple controls, since we'll be adding new ones
  int controlCount;
  EN_API_CHECK( EN_getcount(_enModel, EN_CONTROLCOUNT, &controlCount), "EN_getcount EN_CONTROLCOUNT" );
  _controlCount = controlCount;
  
  // create lookup maps for name->index
  for (int iNode=1; iNode <= nodeCount; iNode++) {
    EN_API_CHECK( EN_getnodeid(_enModel, iNode, enName), "EN_getnodeid" );
    // and keep track of the epanet-toolkit index of this element
    _nodeIndex[string(enName)] = iNode;
  }
  
  for (int iLink = 1; iLink <= linkCount; iLink++) {
    EN_API_CHECK(EN_getlinkid(_enModel, iLink, enName), "EN_getlinkid");
    // keep track of this element index
    _linkIndex[string(enName)] = iLink;
  }
  
  // get the valve types
  for(Valve::_sp v : this->valves()) {
    int enIdx = _linkIndex[v->name()];
    int type = EN_PIPE;
    EN_getlinktype(_enModel, enIdx, &type);
    if (type == EN_PIPE) {
      // should not happen
      continue;
    }
    v->valveType = (int)type;
  }
  
  // consistency checking (should add more)
  for(Node::_sp n : this->nodes()) {
    double elev;
    auto en_idx = _nodeIndex[n->name()];
    EN_getnodevalue(_enModel, en_idx, EN_ELEVATION, &elev);
    if ( abs(elev - n->elevation()) > SMALL) {
      cout << "ERROR: Database elevation inconsistent with model for node " << n->name() << EOL;
      auto badElevationErr = string("Database elevation inconsistent with model for node ") + n->name();
      throw(badElevationErr);
    }
  }
  
  cout << "copying comments" << endl;
  // copy my comments into the model
  for(Node::_sp n : this->nodes()) {
    this->setComment(n, n->userDescription());
  }
  for(Link::_sp l : this->links()) {
    this->setComment(l, l->userDescription());
  }
  
}

void EpanetModel::useEpanetFile(const std::string& filename) {
  EN_Project model;
  EN_createproject(&model);
  
  try {
    // set up temp path for report file so EPANET does not mess with stdout buffer
    boost::filesystem::path rptPath = boost::filesystem::temp_directory_path();
    rptPath /= "en_report.txt";
    
    cout << rptPath << endl;
    
    EN_API_CHECK( EN_open(model, (char*)filename.c_str(), (char*)rptPath.c_str(), (char*)""), "EN_open" );
    
  } catch (const std::string& errStr) {
    cerr << "model not formatted correctly. " << errStr << endl;
    throw "model not formatted correctly. " + errStr;
  }
  
  this->useEpanetModel(model, filename);
  
}

void EpanetModel::initEngine() {
  if (_enOpened) {
    EN_API_CHECK(EN_initH(_enModel, 10), "EN_initH");
    EN_API_CHECK(EN_initQ(_enModel, EN_NOSAVE), "EN_initQ");
    return;
  }
  try {
    EN_API_CHECK(EN_openH(_enModel), "EN_openH");
    EN_API_CHECK(EN_initH(_enModel, 10), "EN_initH");
    EN_API_CHECK(EN_openQ(_enModel), "EN_openQ");
    EN_API_CHECK(EN_initQ(_enModel, EN_NOSAVE), "EN_initQ");
  } catch (...) {
    cerr << "warning: epanet opened improperly" << endl;
  }
  this->applyInitialQuality();
  _enOpened = true;
}

void EpanetModel::closeEngine() {
  if (_enOpened) {
    try {
      EN_API_CHECK( EN_closeQ(_enModel), "EN_closeQ");
      EN_API_CHECK( EN_closeH(_enModel), "EN_closeH");
    } catch (...) {
      cerr << "warning: epanet closed improperly" << endl;
    }
    _enOpened = false;
  }
  
}


void EpanetModel::createRtxWrappers() {
  
  int curveCount, nodeCount, tankCount, linkCount;
  
  try {
    EN_API_CHECK( EN_getcount(_enModel, EN_CURVECOUNT, &curveCount), "EN_getcount EN_CURVECOUNT");
    EN_API_CHECK( EN_getcount(_enModel, EN_NODECOUNT, &nodeCount), "EN_getcount EN_NODECOUNT" );
    EN_API_CHECK( EN_getcount(_enModel, EN_TANKCOUNT, &tankCount), "EN_getcount EN_TANKCOUNT" );
    EN_API_CHECK( EN_getcount(_enModel, EN_LINKCOUNT, &linkCount), "EN_getcount EN_LINKCOUNT" );
  } catch (...) {
    throw "Could not create wrappers";
  }
  
  map<int,Curve::_sp> namedCurves;
  
  for (int iCurve = 1; iCurve <= curveCount; ++iCurve) {
    double *xVals, *yVals;
    int nPoints;
    char buf[1024];
    int err = 0;
    err = EN_getcurvelen(_enModel, iCurve, &nPoints);
    xVals = (double*)calloc(nPoints, sizeof(double));
    yVals = (double*)calloc(nPoints, sizeof(double));
    
    err = EN_getcurve(_enModel, iCurve, buf, &nPoints, xVals, yVals);
    
    if (err) {
      throw("could not find curve " + to_string(iCurve));
    }  
    
    map<double,double> curveData;
    for (int iPoint = 0; iPoint < nPoints; ++iPoint) {
      curveData[xVals[iPoint]] = yVals[iPoint];
    }
    
    Curve::_sp newCurve( new Curve );
    newCurve->curveData = curveData;
    newCurve->inputUnits = TSF_DIMENSIONLESS;
    newCurve->outputUnits = TSF_DIMENSIONLESS;
    newCurve->name = string(buf);
    
    this->addCurve(newCurve);
    namedCurves[iCurve] = newCurve;
    
    free(xVals);
    free(yVals);
  }
  
  
  
  // create nodes
  for (int iNode=1; iNode <= nodeCount; iNode++) {
    char enName[RTX_MAX_CHAR_STRING];
    double x,y,z;         // rtx coordinates
    int nodeType;         // epanet node type code
    string nodeName, comment;
    Junction::_sp newJunction;
    Reservoir::_sp newReservoir;
    Tank::_sp newTank;
    char enComment[MAXMSG+1];
    
    // get relevant info from EPANET toolkit
    EN_API_CHECK( EN_getnodeid(_enModel, iNode, enName), "EN_getnodeid" );
    EN_API_CHECK( EN_getnodevalue(_enModel, iNode, EN_ELEVATION, &z), "EN_getnodevalue EN_ELEVATION");
    EN_API_CHECK( EN_getnodetype(_enModel, iNode, &nodeType), "EN_getnodetype");
    EN_API_CHECK( EN_getcoord(_enModel, iNode, &x, &y), "EN_getcoord");
    EN_API_CHECK( EN_getcomment(_enModel, EN_NODE, iNode, enComment), "EN_getnodecomment");
    
    nodeName = string(enName);
    comment = string(enComment);
    double minLevel = 0, maxLevel = 0, tankDiam = 0;
    
    switch (nodeType) {
      case EN_TANK:
      {
        newTank.reset( new Tank(nodeName) );
        // get tank geometry from epanet and pass it along
        // todo -- geometry
        
        addTank(newTank);
        
        EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_MAXLEVEL, &maxLevel), "EN_getnodevalue(EN_MAXLEVEL)");
        EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_MINLEVEL, &minLevel), "EN_getnodevalue(EN_MINLEVEL)");
        newTank->setMinMaxLevel(minLevel, maxLevel);

        EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_TANKDIAM, &tankDiam), "EN_getnodevalue(EN_TANKDIAM)");
        newTank->setEnProperties(0.0, tankDiam);

        newTank->level()->setUnits(headUnits());
        newTank->flowCalc()->setUnits(flowUnits());
        newTank->volumeCalc()->setUnits(volumeUnits());
        newTank->flow()->setUnits(flowUnits());
        newTank->volume()->setUnits(volumeUnits());
        
        Curve::_sp volumeCurve;
        double volumeCurveIndex;
        EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_VOLCURVE, &volumeCurveIndex), "EN_getnodevalue EN_VOLCURVE");
        
        if (volumeCurveIndex > 0) {
          // curved tank
          volumeCurve = namedCurves[volumeCurveIndex];
        }
        else {
          // it's a cylindrical tank - invent a curve
          double minVolume, maxVolume;
          double minLevel, maxLevel;
          
          EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_MAXLEVEL, &maxLevel), "EN_MAXLEVEL");
          EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_MINLEVEL, &minLevel), "EN_MINLEVEL");
          EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_MINVOLUME, &minVolume), "EN_MINVOLUME");
          EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_MAXVOLUME, &maxVolume), "EN_MAXVOLUME");
          
          volumeCurve.reset( new Curve );
          volumeCurve->curveData[minLevel] = minVolume;
          volumeCurve->curveData[maxLevel] = maxVolume;
          
          stringstream ss;
          ss << "Tank " << newTank->name() << " Cylindrical Curve";
          volumeCurve->name = ss.str();
          this->addCurve(volumeCurve); // unnamed curve: add cylindrical curve to list
        }
        
        volumeCurve->inputUnits = this->headUnits();
        volumeCurve->outputUnits = this->volumeUnits();
        
        // set tank geometry
        newTank->setGeometry(volumeCurve);
        
        newJunction = newTank;
        break;
      }
      case EN_RESERVOIR:
        newReservoir.reset( new Reservoir(nodeName) );
        addReservoir(newReservoir);
        newJunction = newReservoir;
        break;
      case EN_JUNCTION:
        newJunction.reset( new Junction(nodeName) );
        addJunction(newJunction);
        break;
      default:
        throw "Node Type Unknown";
    } // switch nodeType
    
    // set units for new element
    newJunction->head()->setUnits(headUnits());
    newJunction->pressure()->setUnits(pressureUnits());
    newJunction->demand()->setUnits(flowUnits());
    newJunction->quality()->setUnits(qualityUnits());
    
    // newJunction is the generic (base-class) pointer to the specific object,
    // so we can use base-class methods to set some parameters.
    newJunction->setElevation(z);
    newJunction->setCoordinates(Node::location_t(x, y));
    
    // Initial quality specified in input data
    double initQual;
    EN_API_CHECK(EN_getnodevalue(_enModel, iNode, EN_INITQUAL, &initQual), "EN_INITQUAL");
    newJunction->state_quality = initQual;
    
    // Base demand is sum of all demand categories, accounting for patterns
    double demand = 0, categoryDemand = 0, avgPatternValue = 0;
    int numDemands = 0, patternIdx = 0;
    EN_API_CHECK( EN_getnumdemands(_enModel, iNode, &numDemands), "EN_getnumdemands()");
    for (int demandIdx = 1; demandIdx <= numDemands; demandIdx++) {
      EN_API_CHECK( EN_getbasedemand(_enModel, iNode, demandIdx, &categoryDemand), "EN_getbasedemand()" );
      EN_API_CHECK( EN_getdemandpattern(_enModel, iNode, demandIdx, &patternIdx), "EN_getdemandpattern()");
      avgPatternValue = 1.0;
      if (patternIdx > 0) { // Not the default "pattern" = 1
        EN_API_CHECK( EN_getaveragepatternvalue(_enModel, patternIdx, &avgPatternValue), "EN_getaveragepatternvalue()");
      }
      demand += categoryDemand * avgPatternValue;
    }
    newJunction->setBaseDemand(demand);
    newJunction->setUserDescription(comment);
    
    
  } // for iNode
  
  // create links
  for (int iLink = 1; iLink <= linkCount; iLink++) {
    char enLinkName[RTX_MAX_CHAR_STRING+1], enFromName[RTX_MAX_CHAR_STRING+1], enToName[RTX_MAX_CHAR_STRING+1], enComment[RTX_MAX_CHAR_STRING+1];
    int enFrom, enTo;
    int linkType;
    double length, diameter, status, rough, mloss, setting, curveIdx;
    string linkName, comment;
    Node::_sp startNode, endNode;
    Pipe::_sp newPipe;
    Pump::_sp newPump;
    Valve::_sp newValve;
    
    // a bunch of epanet api calls to get properties from the link
    EN_API_CHECK(EN_getlinkid(_enModel, iLink, enLinkName), "EN_getlinkid");
    EN_API_CHECK(EN_getlinktype(_enModel, iLink, &linkType), "EN_getlinktype");
    EN_API_CHECK(EN_getlinknodes(_enModel, iLink, &enFrom, &enTo), "EN_getlinknodes");
    EN_API_CHECK(EN_getnodeid(_enModel, enFrom, enFromName), "EN_getnodeid - enFromName");
    EN_API_CHECK(EN_getnodeid(_enModel, enTo, enToName), "EN_getnodeid - enToName");
    EN_API_CHECK(EN_getlinkvalue(_enModel, iLink, EN_DIAMETER, &diameter), "EN_getlinkvalue EN_DIAMETER");
    EN_API_CHECK(EN_getlinkvalue(_enModel, iLink, EN_LENGTH, &length), "EN_getlinkvalue EN_LENGTH");
    EN_API_CHECK(EN_getlinkvalue(_enModel, iLink, EN_INITSTATUS, &status), "EN_getlinkvalue EN_STATUS");
    EN_API_CHECK(EN_getlinkvalue(_enModel, iLink, EN_ROUGHNESS, &rough), "EN_getlinkvalue EN_ROUGHNESS");
    EN_API_CHECK(EN_getlinkvalue(_enModel, iLink, EN_MINORLOSS, &mloss), "EN_getlinkvalue EN_MINORLOSS");
    EN_API_CHECK(EN_getlinkvalue(_enModel, iLink, EN_INITSETTING, &setting), "EN_getlinkvalue EN_INITSETTING");
    EN_API_CHECK(EN_getcomment(_enModel, EN_LINK, iLink, enComment), "EN_getlinkcomment");
    
    linkName = string(enLinkName);
    comment = string(enComment);
    // get node pointers
    startNode = nodeWithName(string(enFromName));
    endNode = nodeWithName(string(enToName));
    
    if (! (startNode && endNode) ) {
      std::cerr << "could not find nodes for link " << linkName << std::endl;
      throw "nodes not found";
    }
    
    
    // create the new specific type and add it.
    // newPipe becomes the generic (base-class) pointer in all cases.
    switch (linkType) {
      case EN_PIPE:
        newPipe.reset( new Pipe(linkName) );
        newPipe->setNodes(startNode, endNode);
        addPipe(newPipe);
        break;
      case EN_PUMP:
        newPump.reset( new Pump(linkName) );
        newPump->setNodes(startNode, endNode);
        newPipe = newPump;
        addPump(newPump);
        
      {
        // has curve?
        int err = EN_getlinkvalue(_enModel, iLink, EN_PUMP_HCURVE, &curveIdx);
        if (err == 0) {
          Curve::_sp pumpCurve = namedCurves[(int)curveIdx];
          if (pumpCurve) {
            pumpCurve->inputUnits = this->flowUnits();
            pumpCurve->outputUnits = this->headUnits();
            newPump->setHeadCurve(pumpCurve);
          }
        }
        err = EN_getlinkvalue(_enModel, iLink, EN_PUMP_ECURVE, &curveIdx);
        if (err == 0) {
          Curve::_sp effCurve = namedCurves[(int)curveIdx];
          if (effCurve) {
            effCurve->inputUnits = this->flowUnits();
            effCurve->outputUnits = TSF_DIMENSIONLESS;
            newPump->setEfficiencyCurve(effCurve);
          }
        }
      }
        break;
      case EN_CVPIPE:
      case EN_PSV:
      case EN_PRV:
      case EN_FCV:
      case EN_PBV:
      case EN_TCV:
      case EN_GPV:
        newValve.reset( new Valve(linkName) );
        newValve->setNodes(startNode, endNode);
        newValve->valveType = (int)linkType;
        newValve->fixedSetting = setting;
        newPipe = newValve;
        addValve(newValve);
        break;
      default:
        std::cerr << "could not find pipe type" << std::endl;
        break;
    } // switch linkType
    
    
    // now that the pipe is created, set some basic properties.
    newPipe->setDiameter(diameter);
    newPipe->setLength(length);
    newPipe->setRoughness(rough);
    newPipe->setMinorLoss(mloss);
    
    if (status == 0) {
      newPipe->setFixedStatus(Pipe::CLOSED);
    }
    
    newPipe->flow()->setUnits(flowUnits());
    newPipe->setUserDescription(comment);
    
    
  } // for iLink
  
}

void EpanetModel::overrideControls() {
  // set up counting variables for creating model elements.
  int nodeCount, tankCount;
  
  try {
    EN_API_CHECK( EN_getcount(_enModel, EN_NODECOUNT, &nodeCount), "EN_getcount EN_NODECOUNT" );
    EN_API_CHECK( EN_getcount(_enModel, EN_TANKCOUNT, &tankCount), "EN_getcount EN_TANKCOUNT" );
    
    // eliminate all patterns and control rules
    
    int nPatterns;
    double sourcePat;
    double zeroPattern[2];
    zeroPattern[0] = 0;
    
    // clear all patterns, set to zero.  we do this to get rid of extra demand patterns set in [DEMANDS],
    // since there's no other way to get to them using the toolkit.
    EN_API_CHECK( EN_getcount(_enModel, EN_PATCOUNT, &nPatterns), "EN_getcount(EN_PATCOUNT)" );
    for (int i=1; i<=nPatterns; i++) {
      EN_API_CHECK( EN_setpattern(_enModel, i, zeroPattern, 1), "EN_setpattern" );
    }
    for(int iNode = 1; iNode <= nodeCount ; iNode++)  {
      // set pattern to index 0 (unity multiplier). this also rids us of any multiple demand patterns.
      EN_API_CHECK( EN_setnodevalue(_enModel, iNode, EN_PATTERN, 0 ), "EN_setnodevalue(EN_PATTERN)" );  // demand patterns to unity
      EN_API_CHECK( EN_setnodevalue(_enModel, iNode, EN_BASEDEMAND, 0. ), "EN_setnodevalue(EN_BASEDEMAND)" );	// set base demand to zero
      // look for a quality source and nullify its existance
      int errCode = EN_getnodevalue(_enModel, iNode, EN_SOURCEPAT, &sourcePat);
      if (errCode != 240 /* == nonexistent source */) {
        EN_API_CHECK( EN_setnodevalue(_enModel, iNode, EN_SOURCETYPE, EN_CONCEN), "EN_setnodevalue(EN_SOURCETYPE)" );
        EN_API_CHECK( EN_setnodevalue(_enModel, iNode, EN_SOURCEQUAL, 0.), "EN_setnodevalue(EN_SOURCEQUAL)" );
        EN_API_CHECK( EN_setnodevalue(_enModel, iNode, EN_SOURCEPAT, 0.), "EN_setnodevalue(EN_SOURCEPAT)" );
      }
    }
    // set the global demand multiplier is unity as well.
    EN_setoption(_enModel, EN_DEMANDPATTERN, 0); // set default pattern to internal
    EN_setoption(_enModel, EN_DEMANDMULT, 1.);
    
    // disregard controls and rules.
    this->disableControls();
  }
  catch(string error) {
    std::cerr << "ERROR: " << error;
    throw RtxException();
  }
  // base class implementation
  Model::overrideControls();
}

#pragma mark - Protected Methods:

std::ostream& EpanetModel::toStream(std::ostream &stream) {
  // epanet-specific printing
  stream << "Epanet Model File: " << endl;
  Model::toStream(stream);
  return stream;
}

#pragma mark Setters

/* setting simulation parameters */


void EpanetModel::setQualityOptions(QualityType qt, const std::string& traceNode) {
  
  int epanet_qualcode = 0;
  switch (qt) {
    case None:
      epanet_qualcode = 0;
      break;
    case Age:
      epanet_qualcode = 2;
      break;
    case Trace:
      epanet_qualcode = 3;
      break;
    default:
      throw logic_error("unknown quality code");
      break;
  }
  char traceNodeId[MAXID+1];
  strncpy(traceNodeId, traceNode.c_str(), MAXID);
  char blank[] = "";
  
  int err = EN_setqualtype(_enModel, epanet_qualcode, blank, blank, traceNodeId);
  try {
    EN_API_CHECK(err, "setQualityOptions");
  } catch (const std::string& err) {
    cerr << "Error setting quality options: " << err << endl;
  }
  
  
  switch (qt) {
    case Model::Age:
      this->setQualityUnits(TSF_HOUR);
      break;
    case Model::Trace:
      this->setQualityUnits(TSF_DIMENSIONLESS);
      break;
    default:
      break;
  }
  
}

Model::QualityType EpanetModel::qualityType() {
  int code = 0, nodeIndex = 0;
  int err = EN_getqualtype(_enModel, &code, &nodeIndex);
  EN_API_CHECK(err, "qualityType");
  
  switch (code) {
    case 0:
      return Model::None;
    case 2:
      return Model::Age;
    case 3:
      return Model::Trace;
    default:
      throw logic_error("unknown quality code");
      break;
  }
  
  return Model::UNKNOWN;
}

std::string EpanetModel::qualityTraceNode() {
  int code = 0, nodeIndex = 0;
  int err = EN_getqualtype(_enModel, &code, &nodeIndex);
  EN_API_CHECK(err, "qualityTraceNode");
  char id[MAXID+1];
  EN_API_CHECK( EN_getnodeid(_enModel, nodeIndex, id), "EN_getnodeid()" );
  string traceNodeId = string(id);
  return traceNodeId;
}

void EpanetModel::setReservoirHead(const string& reservoir, double level) {
  setNodeValue(EN_TANKLEVEL, reservoir, level);
}

void EpanetModel::setReservoirQuality(const string& reservoir, double quality) {
  setNodeValue(EN_INITQUAL, reservoir, quality); // set initquality in case setpoint is lower than old value
  setNodeValue(EN_SOURCETYPE, reservoir, CONCEN);
  setNodeValue(EN_SOURCEQUAL, reservoir, quality);

  // if this is a tank, we must override the tank's concentration using private EPANET internals.
  const int nJuncs = _enModel->network.Njuncs;
  const int nodeIndex = _nodeIndex[reservoir];
  if (nodeIndex > nJuncs) {
    Stank *tanks = _enModel->network.Tank;
    tanks[nodeIndex - nJuncs].C = quality;
    _enModel->quality.NodeQual[nodeIndex] = quality / _enModel->Ucf[QUALITY];
  }
  
}

void EpanetModel::setTankLevel(const string& tank, double level) {
  // just refer to the reservoir method, since in epanet they are the same thing.
  setReservoirHead(tank, level);
}

void EpanetModel::setJunctionDemand(const string& junction, double demand) {
  int nodeIndex = _nodeIndex[junction];
  // Junction demand is total demand - so deal with multiple categories by clearing them out.
  int numDemands = 0;
  EN_API_CHECK( EN_getnumdemands(_enModel, nodeIndex, &numDemands), "EN_getnumdemands()");
  if (numDemands == 0) {
    cerr << "epanet offers no base demand for this junction: " << junction << endl;
    return;
  }
  // First demand category is the default one... per EPANET convention... and the one RTX wants to manipulate.
  EN_API_CHECK( EN_setbasedemand(_enModel, nodeIndex, 1, demand), "EN_setbasedemand()" );
  // and then set all other demand category multipliers to zero
  for (int demandIdx = 2; demandIdx <= numDemands; demandIdx++) {
    EN_API_CHECK( EN_setbasedemand(_enModel, nodeIndex, demandIdx, 0.0), "EN_setbasedemand()" );
  }
  
}

void EpanetModel::setJunctionQuality(const std::string& junction, double quality) {
  // todo - add more source types, depending on time series dimension?
  setNodeValue(EN_INITQUAL, junction, quality); // set initquality in case setpoint is lower than old value
  setNodeValue(EN_SOURCETYPE, junction, FLOWPACED);
  setNodeValue(EN_SOURCEQUAL, junction, quality);
}

void EpanetModel::setPipeStatus(const string& pipe, Pipe::status_t status) {
  setLinkValue(EN_STATUS, pipe, status);
}

void EpanetModel::setPipeStatusControl(const std::string& pipe, Pipe::status_t status, enableControl_t enableStatus) {
  int linkIndex = _linkIndex[pipe];
  int enEnableStatus = (enableStatus == enable) ? 1 : 0;
  double en_status = (status == Pipe::status_t::CLOSED) ? EN_SET_CLOSED : EN_SET_OPEN;

  if (_statusControlIndex.count(pipe) == 0) {
    // if this element doesn't have a control, add one
    int cindex;
    EN_API_CHECK(EN_addcontrol(_enModel, EN_TIMER, linkIndex, (EN_API_FLOAT_TYPE)en_status, 0, (EN_API_FLOAT_TYPE)0.0, &cindex), "EN_addcontrol");
    _statusControlIndex[pipe] = cindex;
    EN_API_CHECK(EN_setcontrolenabled(_enModel, cindex, enEnableStatus), "EN_setControlEnabled");
  }
  else {
    // set the control
    int cindex = _statusControlIndex[pipe];
    EN_API_CHECK(EN_setcontrol(_enModel, cindex, EN_TIMER, linkIndex, (EN_API_FLOAT_TYPE)en_status, 0, (EN_API_FLOAT_TYPE)0.0), "EN_setcontrol");
    EN_API_CHECK(EN_setcontrolenabled(_enModel, cindex, enEnableStatus), "EN_setControlEnabled");
  }
}

void EpanetModel::setPumpStatus(const string& pump, Pipe::status_t status) {
  // call the setPipeStatus method, since they are the same in epanet.
  setPipeStatus(pump, status);
}

void EpanetModel::setPumpStatusControl(const string& pump, Pipe::status_t status, enableControl_t enableStatus) {
  // call the setPipeStatusControl method, since they are the same in epanet.
  setPipeStatusControl(pump, status, enableStatus);
}

void EpanetModel::setPumpSetting(const string& pump, double setting) {
  setLinkValue(EN_SETTING, pump, setting);
}

void EpanetModel::setPumpSettingControl(const string& pump, double setting, enableControl_t enableStatus) {
  int linkIndex = _linkIndex[pump];
  int enEnableStatus = (enableStatus == enable) ? 1 : 0;

  if (_settingControlIndex.count(pump) == 0) {
    // if this element doesn't have a control, add one
    int cindex;
    EN_API_CHECK(EN_addcontrol(_enModel, EN_TIMER, linkIndex, (EN_API_FLOAT_TYPE)setting, 0, (EN_API_FLOAT_TYPE)0.0, &cindex), "EN_addcontrol");
    _settingControlIndex[pump] = cindex;
    EN_API_CHECK(EN_setcontrolenabled(_enModel, cindex, enEnableStatus), "EN_setControlEnabled");
  }
  else {
    // set the control
    int cindex = _settingControlIndex[pump];
    try {
      EN_API_CHECK(EN_setcontrol(_enModel, cindex, EN_TIMER, linkIndex, (EN_API_FLOAT_TYPE)setting, 0, (EN_API_FLOAT_TYPE)0.0), "EN_setcontrol");
      EN_API_CHECK(EN_setcontrolenabled(_enModel, cindex, enEnableStatus), "EN_setControlEnabled");
    } catch (const std::string& errorMessage) {
      stringstream ss;
      ss << std::string(errorMessage) << EOL;
      this->logLine(ss.str());
    }
    
  }
}

void EpanetModel::setValveSetting(const string& valve, double setting) {
  setLinkValue(EN_SETTING, valve, setting);
}

void EpanetModel::setValveSettingControl(const string& valve, double setting, enableControl_t enableStatus) {
  // call the setPumpSettingControl method, since they are the same in epanet.
  setPumpSettingControl(valve, setting, enableStatus);
}

#pragma mark Getters

double EpanetModel::junctionDemand(const string &junction) {
  return getNodeValue(EN_DEMAND, junction);
}

double EpanetModel::junctionHead(const string &junction) {
  return getNodeValue(EN_HEAD, junction);
}
double EpanetModel::junctionPressure(const string &junction) {
  return getNodeValue(EN_PRESSURE, junction);
}

double EpanetModel::junctionQuality(const string &junction) {
  return getNodeValue(EN_QUALITY, junction);
}

double EpanetModel::tankInletQuality(const string& tank) {
  
  // approximation: get flow in adjacent links, any quality going into the tank can be average flow-weighted.
  
  auto t = dynamic_pointer_cast<Tank>(this->nodeWithName(tank));
  double qualityIn = 0.0;
  double totalFlowIn = 0.0;
  
  for (auto l : t->links()) {
    auto p = dynamic_pointer_cast<Pipe>(l);
    auto flow = p->state_flow;
    
    // ignore very small flows.
    if (fabs(flow) < SMALL) {
      continue;
    }
    
    // if flow is into the tank from this link.
    // create a flow-weighted sum of qualities into the tank.

    // flow towards the tank
    if (p->to() == t && flow > 0) {
      qualityIn += dynamic_pointer_cast<Junction>(p->from())->state_quality * flow;
      totalFlowIn += flow;
    }
    // also flow towards the tank
    else if (p->from() == t && flow < 0) {
      qualityIn += dynamic_pointer_cast<Junction>(p->to())->state_quality * (-flow);
      totalFlowIn += (-flow);
    }
  }
  // complete the flow-weighted averaging.
  qualityIn = qualityIn / totalFlowIn;
  
  if (qualityIn == 0) {
    return NAN;
  }
  else {
    return qualityIn;
  }
}

double EpanetModel::reservoirLevel(const string &reservoir) {
  return getNodeValue(EN_TANKLEVEL, reservoir);
}

double EpanetModel::tankLevel(const string &tank) {
  return junctionHead(tank) - nodeWithName(tank)->elevation(); // node elevation & head in same Epanet units
}

double EpanetModel::tankVolume(const string& tank) {
  return getNodeValue(EN_TANKVOLUME, tank);
}

double EpanetModel::tankFlow(const string& tank) {
  return getNodeValue(EN_DEMAND, tank);
}

double EpanetModel::pipeFlow(const string &pipe) {
  return getLinkValue(EN_FLOW, pipe);
}
double EpanetModel::pipeStatus(const string &pipe) {
  return getLinkValue(EN_STATUS, pipe);
}
double EpanetModel::pipeSetting(const string &pipe) {
  return getLinkValue(EN_SETTING, pipe);
}

double EpanetModel::pipeEnergy(const string &name) {
  return getLinkValue(EN_ENERGY, name);
}

#pragma mark - Sim options
void EpanetModel::enableControls() {
  for (int i = 1; i <= _controlCount; ++i) {
    EN_setcontrolenabled(_enModel, i, EN_TRUE);
  }
  
  int nC;
  EN_getcount(_enModel, EN_RULECOUNT, &nC);
  for (int i = 1; i <= nC; ++i) {
    EN_setruleenabled(_enModel, i, EN_TRUE);
  }
}

void EpanetModel::disableControls() {
  for (int i = 1; i <= _controlCount; ++i) {
    EN_setcontrolenabled(_enModel, i, EN_FALSE);
  }
  
  int nC;
  EN_getcount(_enModel, EN_RULECOUNT, &nC);
  for (int i = 1; i <= nC; ++i) {
    EN_setruleenabled(_enModel, i, EN_FALSE);
  }
}



#pragma mark Simulation Methods

bool EpanetModel::solveSimulation(time_t time) {

  bool success = true;
  long timestep;
  int errorCode;
  
  // set the current epanet-time to zero, since we override epanet-time.
  setCurrentSimulationTime( time );
  EN_API_CHECK(EN_settimeparam(_enModel, EN_HTIME, 0), "EN_settimeparam(EN_HTIME)");
  EN_API_CHECK(EN_settimeparam(_enModel, EN_QTIME, 0), "EN_settimeparam(EN_QTIME)");
  // solve the hydraulics
  errorCode = EN_runH(_enModel, &timestep);
  // check for success
  success = this->_didConverge(time, errorCode);
  
  if (errorCode > 0) {
    char errorMsg[256];
    EN_geterror(errorCode, errorMsg, 255);
    struct tm * timeinfo = localtime (&time);
    stringstream ss;
    ss << std::string(errorMsg) << " :: " << asctime(timeinfo);
    this->logLine(ss.str());
  }
  
  if (errorCode == 110) {
    // ill conditioning can be helped by resetting some things
    this->applyInitialTankLevels();
    this->closeEngine();
    this->initEngine();
    errorCode = EN_runH(_enModel, &timestep);
    if (errorCode > 0) {
      char errorMsg[256];
      EN_geterror(errorCode, errorMsg, 255);
      struct tm * timeinfo = localtime (&time);
      stringstream ss;
      ss << "Re-setting did not help in this case... " << std::string(errorMsg) << " :: " << asctime(timeinfo);
      this->logLine(ss.str());
    }
  }
  
  // how to deal with lack of hydraulic convergence here - reset boundary/initial conditions?
  if (this->shouldRunWaterQuality()) {
    EN_API_CHECK(EN_runQ(_enModel, &timestep), "EN_runQ");
  }
  

  if (false) {
    // log some info like DMA demands
    for (auto dma : this->dmas()) {
      double entotal = 0, rtxtotal = 0;
      auto dmaName = dma->name();
      for (auto j : dma->junctions()) {
        if (j->type() != Element::JUNCTION) {
          continue;
        }
        auto rtxDemand = j->state_demand;
        double enDemand = 0;
        int nodeIndex = enIndexForJunction(j);
        EN_API_CHECK(EN_getnodevalue(_enModel, nodeIndex, EN_DEMAND, &enDemand), "EN_getnodevalue EN_DEMAND");
        cout << "junction " << j->name() << " -- " << rtxDemand << " :: " << enDemand << endl;
        entotal += enDemand;
        rtxtotal += rtxDemand;
      }
      cout << endl << endl << "DMA: " << dma->name() << " -- " << rtxtotal << " :: " << entotal << endl << endl;
    }
  }

  return success;
}

time_t EpanetModel::nextHydraulicStep(time_t time) {
  if ( time != currentSimulationTime() ) {
    // todo - throw something?
    cerr << "time not synchronized!" << endl;
  }
  // get the time of the next hydraulic event (according to the simulation)
  time_t nextTime = time;
  // re-set the epanet engine's hydstep parameter to the original value,
  // so that the step length figurer-outerer works.
  int actualTimeStep = hydraulicTimeStep();
  this->setHydraulicTimeStep(actualTimeStep);
  
  // get time to next hydraulic event
  int eventType;
  long duration = 0;
  int elementIndex = 0;
  EN_API_CHECK(EN_timetonextevent(_enModel, &eventType, &duration, &elementIndex), "EN_timeToNextEvent");
  nextTime += duration;
  
  if (eventType == EN_STEP_TANKEVENT || eventType == EN_STEP_CONTROLEVENT) {
    
    string elementTypeStr("");
    string elementDescStr("");
    
    if (eventType == EN_STEP_TANKEVENT) {
      elementTypeStr = "Tank";
      char id[MAXID+1];
      EN_API_CHECK( EN_getnodeid(_enModel, elementIndex, id), "EN_getnodeid()" );
      elementDescStr = string(id);
    }
    else {
      elementTypeStr = "Control";
      elementDescStr = "index " + to_string(elementIndex) + " :: ";
      
      int controlType, linkIndex, nodeIndex;
      double setting, level;
      EN_getcontrol(_enModel, elementIndex, &controlType, &linkIndex, &setting, &nodeIndex, &level);
      //
      //#define EN_LOWLEVEL     0   /* Control types.  */
      //#define EN_HILEVEL      1   /* See ControlType */
      //#define EN_TIMER        2   /* in TYPES.H.     */
      //#define EN_TIMEOFDAY    3
      
      char linkName[1024];
      EN_getlinkid(_enModel, linkIndex, linkName);
      
      string nodeOrTime("");
      if (nodeIndex == 0) {
        nodeOrTime = "Time";
      }
      else {
        char nodeId[1024];
        EN_getnodeid(_enModel, nodeIndex, nodeId);
        nodeOrTime = string(nodeId);
      }
      
      stringstream controlString;
      switch (controlType) {
        case 0:
          controlString << "Low Level: "<< linkName << "= " << setting << " when " << nodeOrTime << " < " << level;
          break;
        case 1:
          controlString << "High Level: " << linkName << " = " << setting << " when " << nodeOrTime << " > " << level;
          break;
        case 2:
          controlString << "Timer: " << linkName << " = " << setting << " when " << nodeOrTime << " = " << level;
          break;
        case 3:
          controlString << "Time of Day: " << linkName << " = " << setting << " when " << nodeOrTime << " = " << level;
          break;
        default:
          break;
      }

      elementDescStr += controlString.str();
      
    }
    
    stringstream ss;
    ss << "INFO: Simulation step restricted to " << duration << " seconds by " << elementTypeStr << " :: " << elementDescStr;
    this->logLine(ss.str());
  }
  
  return nextTime;
}

// evolve tank levels
void EpanetModel::stepSimulation(time_t time) {
  int step = (int)(time - this->currentSimulationTime());
  long qstep = step;
  
  //std::cout << "set step to: " << step << std::endl;
  
  long computedStep = 0;
  
  int actualStep = this->hydraulicTimeStep();
  this->setHydraulicTimeStep(step);
  EN_API_CHECK( EN_nextH(_enModel, &computedStep), "EN_nextH()" );
  
  if (this->shouldRunWaterQuality()) {
    EN_API_CHECK(EN_nextQ(_enModel, &qstep), "EN_nextQ");
  }
  
  if (step != computedStep) {
    // it's an intermediate step
    stringstream ss;
    ss << "ERROR: Simulation step used for updating tank levels different than expected: set " << step << " and simulated " << computedStep;
    this->logLine(ss.str());
  }
  
  this->setHydraulicTimeStep(actualStep);
  setCurrentSimulationTime( currentSimulationTime() + step );
}

int EpanetModel::iterations(time_t time) {
  double iterations;
  EN_API_CHECK( EN_getstatistic(_enModel, EN_ITERATIONS, &iterations), "EN_getstatistic(EN_ITERATIONS)");
  return iterations;
}

double EpanetModel::relativeError(time_t time) {
  double relativeError;
  EN_API_CHECK( EN_getstatistic(_enModel, EN_RELATIVEERROR, &relativeError), "EN_getstatistic(EN_RELATIVEERROR)");
  return relativeError;
}

bool EpanetModel::_didConverge(time_t time, int errorCode) {
  // return true if the simulation did converge
  EN_API_FLOAT_TYPE accuracy;
  
  EN_API_CHECK( EN_getoption(_enModel, EN_ACCURACY, &accuracy), "EN_getoption");
  bool illcondition = errorCode == 101 || errorCode == 110; // 101 is memory issue, 110 is illconditioning
  bool unbalanced = relativeError(time) > accuracy;
  if (illcondition || unbalanced) {
    return false;
  }
  else {
    return true;
  }
}


void EpanetModel::setHydraulicTimeStep(int seconds) {
  EN_API_CHECK( EN_settimeparam(_enModel, EN_DURATION, (long)seconds), "EN_settimeparam(EN_DURATION)" );
//  EN_API_CHECK( EN_settimeparam(_enModel, EN_REPORTSTEP, (long)seconds), "EN_settimeparam(EN_REPORTSTEP)" );
  EN_API_CHECK( EN_settimeparam(_enModel, EN_HYDSTEP, (long)seconds), "EN_settimeparam(EN_HYDSTEP)" );
  // base class method
  Model::setHydraulicTimeStep(seconds);
  
}

void EpanetModel::setQualityTimeStep(int seconds) {
  EN_API_CHECK( EN_settimeparam(_enModel, EN_QUALSTEP, (long)seconds), "EN_settimeparam(EN_QUALSTEP)");
  // call base class
  Model::setQualityTimeStep(seconds);
}

void EpanetModel::applyInitialQuality() {
  if (!_enOpened) {
    DebugLog << "Could not apply initial quality conditions; engine not opened" << EOL;
    return;
  }
  EN_API_CHECK(EN_closeQ(_enModel), "EN_closeQ");
  EN_API_CHECK(EN_openQ(_enModel), "EN_openQ");
  
  // Junctions
  for(Junction::_sp junc : this->junctions()) {
    double qual = junc->state_quality;
    int iNode = _nodeIndex[junc->name()];
    EN_API_CHECK(EN_setnodevalue(_enModel, iNode, EN_INITQUAL, qual), "EN_setnodevalue - EN_INITQUAL");
  }
  
  // Tanks
  for(Tank::_sp tank : this->tanks()) {
    double qual = tank->state_quality;
    int iNode = _nodeIndex[tank->name()];
    EN_API_CHECK(EN_setnodevalue(_enModel, iNode, EN_INITQUAL, qual), "EN_setnodevalue - EN_INITQUAL");
  }
  
  // reservoirs
  for(auto r: this->reservoirs()) {
    double q = r->state_quality;
    int iNode = _nodeIndex[r->name()];
    EN_API_CHECK(EN_setnodevalue(_enModel, iNode, EN_INITQUAL, q), "EN_setnodevalue - EN_INITIALQUAL");
  }
  
  EN_API_CHECK(EN_initQ(_enModel, EN_NOSAVE), "ENinitQ");
}

void EpanetModel::applyInitialTankLevels() {
  if (!_enOpened) {
    DebugLog << "Could not apply initial tank conditions; engine not opened" << EOL;
    return;
  }
  
  EN_API_CHECK(EN_closeH(_enModel), "EN_closeH");
  EN_API_CHECK(EN_openH(_enModel), "EN_openH");
  
  // Tanks
  for(Tank::_sp tank : this->tanks()) {
    double level = tank->state_level;
    int iNode = _nodeIndex[tank->name()];
    try {
      EN_API_CHECK(EN_setnodevalue(_enModel, iNode, EN_TANKLEVEL, level), "EN_setnodevalue - EN_TANKLEVEL");
    } catch (const std::string& err) {
      cerr << "Error setting tank initial level: " << tank->name() << " : " << level << endl;
      cerr << "--> min/max levels are: " << tank->minLevel() << " - " << tank->maxLevel() << endl;
      cerr << "--> setting level to closest min/max value for desired..." << endl;
      if (tank->minLevel() > level) {
        level = tank->minLevel();
      }
      else if (tank->maxLevel() < level) {
        level = tank->maxLevel();
      }
      EN_API_CHECK(EN_setnodevalue(_enModel, iNode, EN_TANKLEVEL, level), "EN_setnodevalue - EN_TANKLEVEL");
    }
  }
  
  EN_API_CHECK(EN_initH(_enModel, 10), "ENinitH");
}

void EpanetModel::updateEngineWithElementProperties(Element::_sp e) {
  
  //! update hydraulic engine representation with full list of properties from this element.
  
  switch (e->type()) {
    case Element::TANK:
    {
      Tank::_sp t = std::dynamic_pointer_cast<Tank>(e);
      this->setNodeValue(EN_MINLEVEL, t->name(), t->minLevel());
      this->setNodeValue(EN_MAXLEVEL, t->name(), t->maxLevel());
    }
    case Element::JUNCTION:
    case Element::RESERVOIR:
    {
      Junction::_sp j = std::dynamic_pointer_cast<Junction>(e);
      this->setNodeValue(EN_ELEVATION, j->name(), j->elevation());
      this->setNodeValue(EN_BASEDEMAND, j->name(), j->baseDemand());
      this->setComment(j, j->userDescription());
    }
      break;
      
    case Element::VALVE:
    case Element::PUMP:
    case Element::PIPE:
    {
      Pipe::_sp p = std::dynamic_pointer_cast<Pipe>(e);
      this->setLinkValue(EN_DIAMETER, p->name(), p->diameter());
      this->setLinkValue(EN_ROUGHNESS, p->name(), p->roughness());
      this->setLinkValue(EN_LENGTH, p->name(), p->length());
      
      // if it is a valve/pump:
      //    if check valve, don't set status or setting
      //    if fixedStatus is closed, set the setting first
      //    if fixedStatus is open, set the setting last.
      
      auto setSetting = [this](Valve::_sp v) {
        this->setLinkValue(EN_INITSETTING, v->name(), v->fixedSetting);
        this->setLinkValue(EN_SETTING, v->name(), v->fixedSetting);
      };
      
      auto setStatus = [this](Valve::_sp v) {
        this->setLinkValue(EN_INITSTATUS, v->name(), v->fixedStatus());
        this->setLinkValue(EN_STATUS, v->name(), v->fixedStatus());
      };
      
      Valve::_sp v = std::dynamic_pointer_cast<Valve>(p);
      if (v && v->valveType != EN_CVPIPE) {
        // if it's a valve... don't set anything on a check valve!
        if (v->fixedStatus() == RTX::Pipe::CLOSED) {
          //setSetting(v);
          setStatus(v);
        }
        else {
          setStatus(v);
          setSetting(v);
        }
      }
      
      this->setComment(p, p->userDescription());
    }
      break;
    default:
      break;
  }
}

/**
 @brief Revert any simulation modifications made to the current in-memory model data, which should never be saved back to the input file (i.e. by making additional valid changes and then saving them)
 */
void EpanetModel::cleanupModelAfterSimulation() {
  // Remove simple controls added for real time simulation
  int nC;
  EN_getcount(_enModel, EN_CONTROLCOUNT, &nC);
  for (int i = nC; i >= _controlCount + 1; --i) {
    EN_API_CHECK(EN_deletecontrol(_enModel, i), "EN_deletecontrol");
  }
  _settingControlIndex.clear();
  _statusControlIndex.clear();
  
  
  for (auto j : this->junctions()) {
    EN_API_CHECK(EN_setnodevalue(_enModel, this->enIndexForJunction(j), EN_BASEDEMAND, j->baseDemand()), "EN_setnodevalue");
  }

  // TODO - revert patterns? back to their previous values
  
  // TODO - other things: initial tank levels? Anything else that creates confusion with diffs?
  
}

#pragma mark -
#pragma mark Internal Private Methods

double EpanetModel::getNodeValue(int epanetCode, const string& node) {
  int nodeIndex = _nodeIndex[node];
  double value;
  EN_API_CHECK(EN_getnodevalue(_enModel, nodeIndex, epanetCode, &value), "EN_getnodevalue");
  return value;
}
void EpanetModel::setNodeValue(int epanetCode, const string& node, double value) {
  int nodeIndex = _nodeIndex[node];
  stringstream s;
  s << "EN_setnodevalue " << node << " : " << value;
  const string str = s.str();
  EN_API_CHECK(EN_setnodevalue(_enModel, nodeIndex, epanetCode, value), str);
}

double EpanetModel::getLinkValue(int epanetCode, const string& link) {
  int linkIndex = _linkIndex[link];
  double value;
  EN_API_CHECK(EN_getlinkvalue(_enModel, linkIndex, epanetCode, &value), "EN_getlinkvalue");
  return value;
}
void EpanetModel::setLinkValue(int epanetCode, const string& link, double value) {
  int linkIndex = _linkIndex[link];
  EN_API_CHECK(EN_setlinkvalue(_enModel, linkIndex, epanetCode, value), "EN_setlinkvalue");
}

void EpanetModel::setComment(Element::_sp element, const std::string& comment)
{
  
  switch (element->type()) {
    case Element::JUNCTION:
    case Element::TANK:
    case Element::RESERVOIR:
    {
      int nodeIndex = _nodeIndex[element->name()];
      EN_API_CHECK(EN_setcomment(_enModel, EN_NODE, nodeIndex, comment.c_str()), "EN_setnodecomment");
    }
      break;
    case Element::PIPE:
    case Element::PUMP:
    case Element::VALVE:
    {
      int linkIndex = _linkIndex[element->name()];
      EN_API_CHECK(EN_setcomment(_enModel, EN_LINK, linkIndex, comment.c_str()), "EN_setlinkcomment");
    }
      break;
    default:
      break;
  }
  
}

void EpanetModel::EN_API_CHECK(int errorCode, string externalFunction) {
  if (errorCode > 10) {
    char errorMsg[256];
    EN_geterror(errorCode, errorMsg, 255);
    this->logLine(string(externalFunction + " :: " + errorMsg));
    throw externalFunction + "::" + std::string(errorMsg);
  }
}


int EpanetModel::enIndexForJunction(Junction::_sp j) {
  if (_nodeIndex.count(j->name()) > 0) {
    return _nodeIndex.at(j->name());
  }
  else {
    return -1;
  }
}

int EpanetModel::enIndexForPipe(Pipe::_sp p) {
  if (_linkIndex.count(p->name()) > 0) {
    return _linkIndex.at(p->name());
  }
  else {
    return -1;
  }
}

