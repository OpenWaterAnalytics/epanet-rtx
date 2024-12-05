#include "EpanetModelExporter.h"
#include "InpTextPattern.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/filesystem.hpp>
#include <iterator>
#include <fstream>
#include <regex>
#include <algorithm>

#include <LagTimeSeries.h>
#include <PointRecordTime.h>
#include <AggregatorTimeSeries.h>

#include "types.h"

using namespace std;
using namespace RTX;
using namespace TSF;
using PointCollection = PointCollection;

#define BR '\n'


namespace RTX {
  class ControlSet {
  public:
    Point status;
    Point setting;
  };
}


enum _epanet_section_t : int {
  none = 0,
  controls,
  rules,
  other
};

const map<string, _epanet_section_t> _epanet_specialSections = {
  {"CONTROLS", controls},
  {"RULES", rules}
};

const map<string, string> _replacements({
  {"rtxdemand_Demand_Boundary", "DBdy"},
  {"Head_Boundary", "HBdy"},
  {"Quality_Boundary", "QBdy"},
  {"rtxhead_Head_Measure", "HMeas"},
  {"Quality_Measure", "QMeas"},
  {"Pressure_Measure", "PMeas"},
  {"Level_Measure", "LMeas"},
  {"Status_Boundary", "StBdy"},
  {"Setting_Boundary", "SBdy"},
  {"Flow_Measure", "FMeas"},
  {"Energy_Measure", "EMeas"},
  {"rtxdma", "dma"},
});

_epanet_section_t _epanet_sectionFromLine(const string& line);
_epanet_section_t _epanet_sectionFromLine(const string& line) {
  if (line.find('[') != string::npos) {
    regex bracketRegEx("\\[(.*)\\]");
    smatch match;
    if (regex_search(line, match, bracketRegEx) && match.size() == 2) {
      string sectionTitle = match[1].str();
      auto loc = _epanet_specialSections.find(sectionTitle);
      if (loc != _epanet_specialSections.end()) {
        return loc->second;
      }
      else {
        return other;
      }
    }
  }
  return none;
}

int _epanet_make_pattern(EN_Project m, TimeSeries::_sp ts, Clock::_sp clock, TimeRange range, const string& patternName, Units patternUnits);
int _epanet_make_pattern(EN_Project m, TimeSeries::_sp ts, Clock::_sp clock, TimeRange range, const string& patternName, Units patternUnits) {
  TimeSeriesFilter::_sp rsDemand(new TimeSeriesFilter);
  rsDemand->setClock(clock);
  rsDemand->setResampleMode(ResampleModeStep);
  rsDemand->setSource(ts);
  PointCollection pc = rsDemand->pointCollection(range);
  pc.convertToUnits(patternUnits);
  
  string pName(patternName);
  boost::replace_all(pName, " ", "_");
  for( auto pair : _replacements){
    boost::replace_all(pName, pair.first, pair.second);
  }
  
  if(pName.length() > MAXID){
    pName = pName.substr(0,MAXID-1);
  }
  
  char *patName = (char*)pName.c_str();
  int returnCode = EN_addpattern(m, patName);
  if(returnCode == 215){
    cout << "Pattern already exists in model." << EOL << flush;
    int attempts = 1;
    while(returnCode == 215){
      string s = to_string(attempts);
      pName = pName.replace(pName.length() - s.length(), s.length(), s);
      patName = (char*)pName.c_str();
      cout << "Attempting to insert: " << patName << EOL << flush;
      returnCode = EN_addpattern(m, patName);
      attempts++;
    }
    cout << "Success" << EOL << flush;
  }
  
  int patIdx;
  size_t len = pc.count();
  EN_getpatternindex(m, patName, &patIdx);
  double *pattern = (double*)calloc(len, sizeof(double));
  int i = 0;
  pc.apply([&](Point& p){
    pattern[i] = p.value;
    ++i;
  });
  EN_setpattern(m, patIdx, pattern, (int)len);
  free(pattern);
  return patIdx;
}


ostream& RTX::operator<<(ostream& stream, RTX::EpanetModelExporter& exporter) {
  return exporter.to_stream(stream);
}



EpanetModelExporter::EpanetModelExporter(EpanetModel::_sp model, TimeRange range, ExportType exportType) : _range(range), _model(model), _exportType(exportType) {
  
}


void EpanetModelExporter::exportModel(EpanetModel::_sp model, TimeRange range, const std::string& dir, bool exportCalibration, ExportType exportType) {
  
  boost::filesystem::path path(dir);
  if (!boost::filesystem::exists(path)) {
    if (!boost::filesystem::create_directory(path)) {
      cerr << "could not create directory for export" << BR;
      return;
    }
  }
  
  const char* fmt = "%Y-%m-%dT%H-%M-%SZ";
  auto startStr = PointRecordTime::utcDateStringFromUnix(range.start,fmt);
  auto endStr = PointRecordTime::utcDateStringFromUnix(range.end,fmt);
  
  stringstream p;
  p << model->name() << "_" << startStr << "_" << endStr << "_" << (exportType == Snapshot ? "snapshot" : "forecast") << ".inp";
  
  auto modelPath = path / p.str();
  
  std::ofstream expFile;
  expFile.open(modelPath.string(), ios_base::out);
  
  if (expFile) {
    EpanetModelExporter e(static_pointer_cast<EpanetModel>(model), range, exportType);
    expFile << e;
  }
  expFile.flush();
  expFile.close();
  
  // calibration files:
  
  ////// pressure
  {
    auto calFile = path / "model_pressure.txt";
    ofstream s;
    s.open(calFile.string());
    if (s) {
      s << ";PRESSURE MEASUREMENTS" << BR;
      s << ";Location    Time    Value" << BR;
      
      for (auto j : model->junctions()) {
        if (j->pressureMeasure()) {
          // element name
          s << ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;" << BR;
          s << "; Junction " << j->name() << " pressure measure" << BR;
          s << j->name() << " ";
          auto pc = j->pressureMeasure()->pointCollection(range);
          pc.convertToUnits(model->pressureUnits());
          auto series = pc.points();
          for (auto &p : series) {
            s << (p.time - range.start)/3600.0 << "  " << p.value << BR;
          }
          s << BR << BR;
        }
      }
      
    }
    s.flush();
    s.close();
  }
  
  ////// head (tank levels)
  {
    auto calFile = path / "model_head.txt";
    ofstream s;
    s.open(calFile.string());
    if (s) {
      s << ";HEAD (TANK LEVEL) MEASUREMENTS" << BR;
      s << ";Location    Time    Value" << BR;
      
      for (auto t : model->tanks()) {
        if (t->headMeasure()) {
          // element name
          s << ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;" << BR;
          s << "; Tank " << t->name() << " head measure" << BR;
          s << t->name() << " ";
          auto pc = t->headMeasure()->pointCollection(range);
          pc.convertToUnits(model->headUnits());
          auto series = pc.points();
          for (auto &p : series) {
            s << (p.time - range.start)/3600.0 << "  " << p.value << BR;
          }
          s << BR << BR;
        }
      }
      
    }
    s.flush();
    s.close();
  }
  
  ////// demand (measured demands)
  {
    auto calFile = path / "model_demand.txt";
    ofstream s;
    s.open(calFile.string());
    if (s) {
      s << ";DEMAND MEASUREMENTS" << BR;
      s << ";Location    Time    Value" << BR;
      
      for (auto j : model->junctions()) {
        if (j->boundaryFlow()) {
          // element name
          s << ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;" << BR;
          s << "; Junction " << j->name() << " demand boundary" << BR;
          s << j->name() << " ";
          auto pc = j->boundaryFlow()->pointCollection(range);
          pc.convertToUnits(model->flowUnits());
          auto series = pc.points();
          for (auto &p : series) {
            s << (p.time - range.start)/3600.0 << "  " << p.value << BR;
          }
          s << BR << BR;
        }
      }
      
    }
    s.flush();
    s.close();
  }
  
  ////// flow
  {
    auto calFile = path / "model_flow.txt";
    ofstream s;
    s.open(calFile.string());
    if (s) {
      s << ";FLOW MEASUREMENTS" << BR;
      s << ";Location    Time    Value" << BR;
      vector<Pipe::_sp> pipes = model->pipes();
      for (auto p : model->pumps()) {
        pipes.push_back(p);
      }
      for (auto v : model->valves()) {
        pipes.push_back(v);
      }
      for (auto p : pipes) {
        if (p->flowMeasure()) {
          // element name
          s << ";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;" << BR;
          s << "; Pipe " << p->name() << " flow measure" << BR;
          s << p->name() << " ";
          auto pc = p->flowMeasure()->pointCollection(range);
          pc.convertToUnits(model->flowUnits());
          auto series = pc.points();
          for (auto &p : series) {
            s << (p.time - range.start)/3600.0 << "  " << p.value << BR;
          }
          s << BR << BR;
        }
      }
      
    }
    s.flush();
    s.close();
  }
  
  ////// quality (where measured?)
  
}




ostream& EpanetModelExporter::to_stream(ostream &stream) {
  
  // create a copy of the project, so that we don't alter important properties of this one.
  string fileName = _model->modelFile();
  EpanetModel::_sp modelTemplate( new EpanetModel(fileName) );
  EN_Project ow_project = modelTemplate->epanetModelPointer();
  
  // the pattern clock matches the hydraulic timestep,
  // and has an offset that makes the first tick occur at
  // the start of simulation.
  Clock::_sp patternClock(new Clock(_model->hydraulicTimeStep(), _range.start));
  
  // simulation time parameters
  EN_settimeparam(ow_project, EN_PATTERNSTEP, (long)_model->hydraulicTimeStep());
  EN_settimeparam(ow_project, EN_DURATION, (long)(_range.duration()));
  
  /***************************************************************/
  // clean up junction demands. 
  // any categories? ignore them by setting their base to zero
  for (auto junction : _model->junctions()) {
    int jIdx = _model->enIndexForJunction(junction);
    // Junction demand is total demand - so deal with multiple categories
    int numDemands = 0;
    EN_getnumdemands(ow_project, jIdx, &numDemands);

    // the first category is the default.
    if (numDemands > 1) {
      for (int demandIdx = 2; demandIdx <= numDemands; demandIdx++) {
        EN_setbasedemand(ow_project, jIdx, demandIdx, 0.0);
      }
    }
    
  }
  
  
  /*******************************************************/
  // get relative base demands for junctions, and set them
  // using the epanet api.
  // for junctions with measured (boundary) demands,
  // set base demand to unity.
  /*******************************************************/
  
  for (auto dma: _model->dmas()) {
    double totalBase = 0;
    // compute total demand
    for (auto junction: dma->junctions()) {
      if (!junction->boundaryFlow()) {
        // baseDemand = 0 at demand boundaries, but ... make sure we ignore them
        totalBase += junction->baseDemand();
      }
    }
    // distribute the demand
    for (auto junction: dma->junctions()) {
      double thisBase = 0;
      if (junction->boundaryFlow()) {
        thisBase = 1;
      }
      else if (totalBase != 0) {
        thisBase = (junction->baseDemand() / totalBase);
      }
      // setnodevalue will set the FIRST category's base demand.
      EN_setnodevalue(ow_project,
                      _model->enIndexForJunction(junction),
                      EN_BASEDEMAND,
                      thisBase );
    }
  }
  
  /*******************************************************/
  // get dma series, put them into epanet patterns
  // link junctions to the dma pattern they should follow.
  // for junctions with boundary demands, use unity pattern.
  /*******************************************************/
  for (auto dma: _model->dmas()) {
    
    TimeSeries::_sp demand;
    
    if (_exportType == Snapshot) {
      // dma pattern is D(t) - DM(t) where D(t) is the DMA demand,
      // and DM(t) the total demand at measured boundaries.
      // This requires that demand boundary base demands = 1
      AggregatorTimeSeries::_sp d(new AggregatorTimeSeries);
      d->addSource(dma->demand(), 1);
      d->setName(dma->name());
      for (auto j: dma->junctions()) {
        if (j->boundaryFlow()) {
          d->addSource(j->boundaryFlow(), -1);
        }
      }
      demand = d;
    }
    else {
      auto d = dma->demand();
      // demand model
      LagTimeSeries::_sp weekAgo(new LagTimeSeries());
      weekAgo->setName(d->name());
      weekAgo->setOffset((time_t)(7*24*3600));
      weekAgo->setSource(d);
      demand = weekAgo;
    }
    
    
    string pName = "rtxdma_" + demand->name();
    boost::replace_all(pName, " ", "_");
    int pIndex = _epanet_make_pattern(ow_project, demand, patternClock, _range, pName, _model->flowUnits());
    
    for (auto j: dma->junctions()) {
      int jPatIdx = (j->boundaryFlow()) ? 0 : pIndex;
      EN_setnodevalue(ow_project, _model->enIndexForJunction(j), EN_PATTERN, jPatIdx);
    }
  }
  
  /*******************************************************/
  // get boundary head series, and put them into epanet patterns
  // and the tell reservoirs which head pattern to use
  // reset the boundary heads to unity
  /*******************************************************/
  if (_exportType == Snapshot) {
    for (auto r: _model->reservoirs()) {
      if (r->headMeasure()) {
        TimeSeries::_sp h = r->headMeasure();
        string pName = "rtxhead_" + h->name();
        boost::replace_all(pName, " ", "_");
        int pIndex = _epanet_make_pattern(ow_project, h, patternClock, _range, pName, _model->headUnits());
        EN_setnodevalue(ow_project, _model->enIndexForJunction(r), EN_PATTERN, pIndex);
        EN_setnodevalue(ow_project, _model->enIndexForJunction(r), EN_TANKLEVEL, 1.0);
      }
    }
  }
  else {
    // only set initial reservoir heads.
    for (auto r: _model->reservoirs()) {
      if (r->headMeasure()) {
        double iHead = r->headMeasure()->pointAtOrBefore(_range.start).value;
        EN_setnodevalue(ow_project, _model->enIndexForJunction(r), EN_TANKLEVEL, iHead);
      }
    }
  }
  
  /*******************************************************/
  // get boundary demand series, and put them into epanet patterns
  // then tell that junction which pattern to use
  /*******************************************************/
  if (_exportType == Snapshot) {
    for (auto j: _model->junctions()) {
      if (j->boundaryFlow()) {
        TimeSeries::_sp demand = j->boundaryFlow();
        string pName = "rtxdemand_" + demand->name();
        boost::replace_all(pName, " ", "_");
        int pIndex = _epanet_make_pattern(ow_project, demand, patternClock, _range, pName, _model->flowUnits());
        EN_setnodevalue(ow_project, _model->enIndexForJunction(j), EN_PATTERN, pIndex);
      }
    }
  }
  
  /*******************************************************/
  // set initial tank levels
  /*******************************************************/
  
  for (auto t: _model->tanks()) {
    if (t->levelMeasure()) {
      double iLevel = t->levelMeasure()->pointAtOrBefore(_range.start).value;
      EN_setnodevalue(ow_project, _model->enIndexForJunction(t), EN_TANKLEVEL, iLevel);
    }
  }
  
  /*******************************************************/
  // save .inp file to temp location
  // copy the file line-by-line into the stream provided
  // intercept the stream during the [CONTROLS] section
  // and insert our own control statements regarding
  // pump, valve, and pipe operation.
  /*******************************************************/
  
  boost::filesystem::path modelPath = boost::filesystem::temp_directory_path();
  modelPath /= "export_rt_model.inp";
  EN_saveinpfile(ow_project, (char*)modelPath.c_str());
  
  ifstream originalFile;
  originalFile.open(modelPath.string());
  if (!originalFile) {
    cerr << "could not open original model file" << BR;
    return stream;
  }
  
  
  /*
   insert header section to explain the model file   
   */
  
  stream << setfill (';') << setw (80) << BR;
  stream << ";;  EPANET-RTX MODEL EXPORT" << BR;
  stream << ";;    - export type: " << (this->_exportType == Snapshot ? "Snapshot" : "Forecast") << BR;
  stream << ";;    - date range: " 
  << "UTC " << PointRecordTime::utcDateStringFromUnix(this->_range.start) 
  << " --> UTC " << PointRecordTime::utcDateStringFromUnix(this->_range.end) << BR << BR;
  
  if (_exportType == Snapshot) {
    stream << ";;  Logical model controls have been removed from this model, " << BR; 
    stream << ";;  and time-based controls derived from data sources have been inserted. " << BR;
    stream << ";;  Demands have been set based on historical data within the date ranges listed above." << BR;
  }
  else {
    stream << ";;  Logical model controls have been preserved in this model. " << BR; 
    stream << ";;  Demands have been set based on user-specified criteria." << BR;
  }
  stream << setfill (';') << setw (80) << "\n\n\n";
  
  
  
  // dump to the new .inp file
  if (_exportType == Snapshot) {
    this->replaceControlsInStream(originalFile, stream);
  }
  else {
    // stream the entire file, no replacements.
    string line;
    while (getline(originalFile, line)) {
      stream << line << BR; // pass through
    }
  }
  
  stream.flush();
  
  originalFile.close();
  boost::filesystem::remove_all(modelPath);
  
  return stream;
}


void EpanetModelExporter::replaceControlsInStream(ifstream &originalFile, ostream &stream) {
  
  for (string line; getline(originalFile, line); ) {
    
    _epanet_section_t section = _epanet_sectionFromLine(line);
    
    if (section == controls) {
      cout << "high-jacking the controls section" << BR;
      // entered [CONTROLS] section.
      // copy the line over, then stream our own statements.
      stream << line << BR << BR << BR;
      vector<Pipe::_sp> pipes = _model->pipes();
      for (auto p : _model->pumps()) {
        pipes.push_back(p);
      }
      for (auto v : _model->valves()) {
        pipes.push_back(v);
      }
      
      for (auto p: pipes) {
        const string name = p->name();
        
        if (!p->settingBoundary() && !p->statusBoundary()) {
          continue;
        }
        
        // make it easy to find any status or setting at a certain time
        map<time_t, ControlSet> controls;
        
        if (p->settingBoundary()) {
          // make sure that the first point is at or before "time zero"
          TimeRange settingRange = _range;
          settingRange.start = p->settingBoundary()->pointAtOrBefore(_range.start).time;
          PointCollection settings = p->settingBoundary()->pointCollection(settingRange).asDelta();
          settings.apply([&](Point& p){
            controls[p.time].setting = p;
          });
        }
        if (p->statusBoundary()) {
          // also back up by one point
          TimeRange statusRange = _range;
          statusRange.start = p->statusBoundary()->pointAtOrBefore(_range.start).time;
          PointCollection statuses = p->statusBoundary()->pointCollection(statusRange).asDelta();
          statuses.apply([&](Point& p){
            controls[p.time].status = p;
          });
        }
        
        // generate control statements
        if (controls.size() > 0) {
          bool isOpen = true;
          stream << BR << "; RTX Time-Based Control for " << name << BR;
          
          Point previousSetting;
          auto c1 = controls.begin();
          previousSetting = c1->second.setting; // initial setting for initially "off" controls
          
          for (auto c: controls) {
            time_t t = c.first;
            ControlSet control = c.second;
            double hrs = (double)(t - _range.start) / (60.*60.);
            hrs = (hrs < 0) ? 0 : hrs;
            if (control.status.isValid) {
              // status update
              isOpen = (control.status.value > 0.);
              // OPEN for a valve means ignore the setting, so don't print an OPEN command if the valve has a setting boundary
              if (p->type() == Element::VALVE && isOpen && p->settingBoundary()) {
                stream << "; ";
              }
              stream << "LINK " << name << " " << (isOpen ? "OPEN" : "CLOSED") << " AT TIME " << hrs << BR;
            }
            if (isOpen) {
              if (control.setting.isValid) {
                // new setting control
                stream << "LINK " << name << " " << std::max(0.0, control.setting.value) << " AT TIME " << hrs << BR;// cache the previous setting
                previousSetting = control.setting;
              }
              else if (control.status.isValid) { // if link is open, and a status is being set...
                // Newly opened - reestablish previous link setting, if possible
                if (previousSetting.isValid) {
                  stream << "LINK " << name << " " << std::max(0.0, previousSetting.value) << " AT TIME " << hrs << BR;
                }
              }
            } // isOpen
            else {
              // is closed, but if there's a valid setting, then remember it just in case we need it.
              if (control.setting.isValid) {
                previousSetting = control.setting;
              }
            }
          } // for c: controls
        }
      }
      // section is still controls, and we may have lingering controls here.
      // ignore lines until a new section is reached.
      while (getline(originalFile, line)) {
        section = _epanet_sectionFromLine(line);
        cout << "skipping the rest of the controls (if any)" << BR;
        if ( section != none) {
          break;
        }
      }
      
      stream << BR << BR;
    } // end if section == controls
    
    if (section == rules) {
      // fast-forward thru rules section
      while (getline(originalFile, line)) {
        section = _epanet_sectionFromLine(line);
        if ( section != none) {
          break;
        }
      }
    } // end if section == rules
    
    
    
    
    switch (section) {
      case other:
        cout << "found section line: " << line << BR;
      case none:
        stream << line << BR; // pass through
        continue; // top of loop
        break;
      default:
        cout << "found section line: " << line << BR;
        break;
    }
    
  }
  stream.flush();
}




