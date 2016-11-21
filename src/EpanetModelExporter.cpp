#include "EpanetModelExporter.h"
#include "InpTextPattern.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/range/join.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/filesystem.hpp>
#include <iterator>
#include <fstream>
#include <regex>
#include <algorithm>

using namespace std;
using namespace RTX;
using PointCollection = TimeSeries::PointCollection;
using boost::join;

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
  controls
};

const map<string, _epanet_section_t> _epanet_specialSections = {
  {"CONTROLS", controls}
};

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
        return none;
      }
    }
  }
  return none;
}

int _epanet_make_pattern(OW_Project *m, TimeSeries::_sp ts, Clock::_sp clock, TimeRange range, const string& patternName, Units patternUnits);
int _epanet_make_pattern(OW_Project *m, TimeSeries::_sp ts, Clock::_sp clock, TimeRange range, const string& patternName, Units patternUnits) {
  TimeSeriesFilter::_sp rsDemand(new TimeSeriesFilter);
  rsDemand->setClock(clock);
  rsDemand->setResampleMode(TimeSeries::TimeSeriesResampleModeStep);
  rsDemand->setSource(ts);
  PointCollection pc = rsDemand->pointCollection(range);
  pc.convertToUnits(patternUnits);
  
  string pName(patternName);
  pName = pName.substr(0,30);
  boost::replace_all(pName, " ", "_");
  char *patName = (char*)pName.c_str();
  OW_addpattern(m, patName);
  int patIdx;
  size_t len = pc.count();
  OW_getpatternindex(m, patName, &patIdx);
  double *pattern = (double*)calloc(len, sizeof(double));
  int i = 0;
  for (auto p: pc.points) {
    pattern[i] = p.value;
    ++i;
  }
  OW_setpattern(m, patIdx, pattern, (int)len);
  free(pattern);
  return patIdx;
}


ostream& RTX::operator<<(ostream& stream, RTX::EpanetModelExporter& exporter) {
  return exporter.to_stream(stream);
}



EpanetModelExporter::EpanetModelExporter(EpanetModel::_sp model, TimeRange range) : _range(range), _model(model) {
  
}




ostream& EpanetModelExporter::to_stream(ostream &stream) {
  
  // create a copy of the project, so that we don't alter important properties of this one.
  string fileName = _model->modelFile();
  EpanetModel::_sp modelTemplate( new EpanetModel(fileName) );
  OW_Project *ow_project = modelTemplate->epanetModelPointer();
  
  // the pattern clock matches the hydraulic timestep,
  // and has an offset that makes the first tick occur at
  // the start of simulation.
  Clock::_sp patternClock(new Clock(_model->hydraulicTimeStep(), _range.start));
  
  // simulation time parameters
  OW_settimeparam(ow_project, EN_PATTERNSTEP, (long)_model->hydraulicTimeStep());
  OW_settimeparam(ow_project, EN_DURATION, (long)(_range.duration()));
  
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
      totalBase += junction->baseDemand();
    }
    // distribute the demand
    for (auto junction: dma->junctions()) {
      double thisBase = 0;
      if (totalBase != 0) {
        thisBase = (junction->boundaryFlow()) ? 1 : (junction->baseDemand() / totalBase);
      }
      OW_setnodevalue(ow_project,
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
    TimeSeries::_sp demand = dma->demand();
    string pName = "rtxdma_" + demand->name();
    boost::replace_all(pName, " ", "_");
    int pIndex = _epanet_make_pattern(ow_project, demand, patternClock, _range, pName, _model->flowUnits());
    
    for (auto j: dma->junctions()) {
      int jPatIdx = (j->boundaryFlow()) ? 0 : pIndex;
      OW_setnodevalue(ow_project, _model->enIndexForJunction(j), EN_PATTERN, jPatIdx);
    }
  }
  
  /*******************************************************/
  // get boundary head series, and put them into epanet patterns
  // and the tell reservoirs which head pattern to use
  // reset the boundary heads to unity
  /*******************************************************/
  for (auto r: _model->reservoirs()) {
    if (r->headMeasure()) {
      TimeSeries::_sp h = r->headMeasure();
      string pName = "rtxhead_" + h->name();
      boost::replace_all(pName, " ", "_");
      int pIndex = _epanet_make_pattern(ow_project, h, patternClock, _range, pName, _model->headUnits());
      OW_setnodevalue(ow_project, _model->enIndexForJunction(r), EN_PATTERN, pIndex);
      OW_setnodevalue(ow_project, _model->enIndexForJunction(r), EN_TANKLEVEL, 1.0);
    }
  }
  
  /*******************************************************/
  // get boundary demand series, and put them into epanet patterns
  // then tell that junction which pattern to use
  /*******************************************************/
  
  for (auto j: _model->junctions()) {
    if (j->boundaryFlow()) {
      TimeSeries::_sp demand = j->boundaryFlow();
      string pName = "rtxdem_" + demand->name();
      boost::replace_all(pName, " ", "_");
      int pIndex = _epanet_make_pattern(ow_project, demand, patternClock, _range, pName, _model->flowUnits());
      OW_setnodevalue(ow_project, _model->enIndexForJunction(j), EN_PATTERN, pIndex);
    }
  }
  
  /*******************************************************/
  // set initial tank levels
  /*******************************************************/
  
  for (auto t: _model->tanks()) {
    if (t->levelMeasure()) {
      double iLevel = t->levelMeasure()->pointAtOrBefore(_range.start).value;
      OW_setnodevalue(ow_project, _model->enIndexForJunction(t), EN_TANKLEVEL, iLevel);
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
  modelPath /= "model.inp";
  OW_saveinpfile(ow_project, modelPath.c_str());
  
  ifstream originalFile;
  originalFile.open(modelPath.string());
  
  string line;
  while (getline(originalFile, line)) {
    _epanet_section_t section = _epanet_sectionFromLine(line);
    if (section == controls) {
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
          TimeSeries::PointCollection settings = p->settingBoundary()->pointCollection(settingRange).asDelta();
          for(const Point& p: settings.points) {
            controls[p.time].setting = p;
          }
        }
        if (p->statusBoundary()) {
          // also back up by one point
          TimeRange statusRange = _range;
          statusRange.start = p->statusBoundary()->pointAtOrBefore(_range.start).time;
          TimeSeries::PointCollection statuses = p->statusBoundary()->pointCollection(statusRange).asDelta();
          for(const Point& p: statuses.points) {
            controls[p.time].status = p;
          }
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
    }
    else {
      stream << line << BR;
    }
  }
  stream.flush();
  originalFile.close();
  boost::filesystem::remove_all(modelPath);
  
  return stream;
}







