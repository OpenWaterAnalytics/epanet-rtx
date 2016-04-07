#include "EpanetModelExporter.h"
#include "InpTextPattern.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/range/join.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <regex>

using namespace std;
using namespace RTX;
using PointCollection = TimeSeries::PointCollection;
using boost::join;

#define BR '\n'

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
  boost::replace_all(pName, " ", "_");
  char *patName = (char*)patternName.c_str();
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
  OW_Project *m = _model->epanetModelPointer();
  
  // the pattern clock matches the hydraulic timestep,
  // and has an offset that makes the first tick occur at
  // the start of simulation.
  Clock::_sp patternClock(new Clock(_model->hydraulicTimeStep(), _range.start));
  
  
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
      double thisBase = (junction->boundaryFlow()) ? 1 : (junction->baseDemand() / totalBase);
      OW_setnodevalue(m,
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
    string pName = demand->name() + "_rtx_dma_demand";
    boost::replace_all(pName, " ", "_");
    int pIndex = _epanet_make_pattern(m, demand, patternClock, _range, pName, _model->flowUnits());
    
    for (auto j: dma->junctions()) {
      int jPatIdx = (j->boundaryFlow()) ? 0 : pIndex;
      OW_setnodevalue(m, _model->enIndexForJunction(j), EN_PATTERN, jPatIdx);
    }
    
  }
  
  
  
  /*******************************************************/
  // get boundary head series, and put them into epanet patterns
  // and the tell reservoirs which head pattern to use
  /*******************************************************/
  for (auto r: _model->reservoirs()) {
    if (r->headMeasure()) {
      TimeSeries::_sp h = r->headMeasure();
      string pName = h->name() + "_rtx_head";
      boost::replace_all(pName, " ", "_");
      int pIndex = _epanet_make_pattern(m, h, patternClock, _range, pName, _model->headUnits());
      OW_setnodevalue(m, _model->enIndexForJunction(r), EN_PATTERN, pIndex);
    }
  }
  
  
  
  /*******************************************************/
  // get boundary demand series, and put them into epanet patterns
  // then tell that junction which pattern to use
  /*******************************************************/
  
  for (auto j: _model->junctions()) {
    if (j->boundaryFlow()) {
      TimeSeries::_sp demand = j->boundaryFlow();
      string pName = demand->name() + "_rtx_demand_boundary";
      boost::replace_all(pName, " ", "_");
      int pIndex = _epanet_make_pattern(m, demand, patternClock, _range, pName, _model->flowUnits());
      OW_setnodevalue(m, _model->enIndexForJunction(j), EN_PATTERN, pIndex);
    }
  }
  
  
  
  /*******************************************************/
  // set initial tank levels
  /*******************************************************/
  
  for (auto t: _model->tanks()) {
    if (t->levelMeasure()) {
      double iLevel = t->levelMeasure()->pointAtOrBefore(_range.start).value;
      OW_setnodevalue(m, _model->enIndexForJunction(t), EN_TANKLEVEL, iLevel);
    }
  }
  
  
  /*******************************************************/
  // save .inp file to temp location
  // copy the file line-by-line into the stream provided
  // intercept the stream during the [CONTROLS] section
  // and insert our own control statements regarding
  // pump, valve, and pipe operation.
  /*******************************************************/
  
  boost::filesystem::path p = boost::filesystem::temp_directory_path();
  
  p /= "model.inp";
  
  OW_saveinpfile(m, p.c_str());
  
  ifstream originalFile;
  originalFile.open(p.string());
  
  string line;
  while (getline(originalFile, line)) {
    _epanet_section_t section = _epanet_sectionFromLine(line);
    if (section == controls) {
      // entered [CONTROLS] section.
      // copy the line over, then stream our own statements.
      stream << line << BR << BR << BR;
      vector<Pipe::_sp> pipes = _model->pipes();
      join(pipes, _model->pumps());
      join(pipes, _model->valves());
      for (auto p: pipes) {
        
        if (p->settingBoundary()) {
          TimeSeries::_sp ts = p->settingBoundary();
          string stmt = InpTextPattern::textControlWithTimeSeries(ts, p->name(), _range.start, _range.end, InpTextPattern::InpControlTypeSetting);
          stream << stmt << BR << BR;
        }
        if (p->statusBoundary()) {
          TimeSeries::_sp ts = p->statusBoundary();
          string stmt = InpTextPattern::textControlWithTimeSeries(ts, p->name(), _range.start, _range.end, InpTextPattern::InpControlTypeStatus);
          stream << stmt << BR << BR;
        }
        
      }
      
    }
    else {
      stream << line << BR;
    }
  }
  stream.flush();
  originalFile.close();
  boost::filesystem::remove_all(p);
  
  return stream;
}







