//
//  ProjectFile.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include <iostream>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/lexical_cast.hpp>

#include "ProjectFile.h"

#include "TimeSeries.h"

#include "Dma.h"

using namespace RTX;
using namespace std;

vector<ProjectFile::ElementSummary> ProjectFile::projectSummary(time_t start, time_t end) {
  // should create dmas before using this method to get reliable summary information about dma boundary flows
  vector<ElementSummary> summary;
  Model::sharedPointer projModel = model();
  vector<Element::sharedPointer> modelElements;
  vector<Dma::sharedPointer> Dmas;
  if (projModel) {
    modelElements = projModel->elements();
    Dmas = projModel->dmas();
  }
  else {
    return summary;
  }
  
  // record and categorize the connected time series
  BOOST_FOREACH(Element::sharedPointer element, modelElements) {
    switch (element->type()) {
      case Element::JUNCTION:
      case Element::TANK:
      case Element::RESERVOIR: {
        Junction::sharedPointer junc;
        junc = boost::static_pointer_cast<Junction>(element);
        if (junc->doesHaveBoundaryFlow()) {
          ElementSummary s;
          s.element = element;
          s.data = junc->boundaryFlow()->rootTimeSeries();
          s.boundaryType = B_DEMAND;
          s.measureType = M_NONE;
          summary.push_back(s);
        }
        if (junc->doesHaveHeadMeasure()) {
          ElementSummary s;
          s.element = element;
          s.data = junc->headMeasure()->rootTimeSeries();
          s.boundaryType = B_NONE;
          s.measureType = M_NONE;
          (element->type() == Element::RESERVOIR) ? (s.boundaryType = B_HEAD) : (s.measureType = M_HEAD);
          summary.push_back(s);
        }
        if (junc->doesHaveQualityMeasure()) {
          ElementSummary s;
          s.element = element;
          s.data = junc->qualityMeasure()->rootTimeSeries();
          s.boundaryType = B_NONE;
          s.measureType = M_QUALITY;
          summary.push_back(s);
        }
        if (junc->doesHaveQualitySource()) {
          ElementSummary s;
          s.element = element;
          s.data = junc->qualitySource()->rootTimeSeries();
          s.boundaryType = B_QUALITY;
          s.measureType = M_NONE;
          summary.push_back(s);
        }
        break;
      }
        
      case Element::PIPE:
      case Element::VALVE:
      case Element::PUMP: {
        Pipe::sharedPointer pipe;
        pipe = boost::static_pointer_cast<Pipe>(element);
        if (pipe->doesHaveFlowMeasure()) {
          ElementSummary s;
          s.element = element;
          s.data = pipe->flowMeasure()->rootTimeSeries();
          s.boundaryType = B_NONE;
          s.measureType = M_FLOW;
          BOOST_FOREACH(Dma::sharedPointer dma, Dmas) {
            if (dma->isMeasuredBoundaryPipe(pipe)) {
              s.measureType = M_DMA_FLOW;
              break;
            }
          }
          summary.push_back(s);
        }
        if (pipe->doesHaveSettingParameter()) {
          ElementSummary s;
          s.element = element;
          s.data = pipe->settingParameter()->rootTimeSeries();
          s.boundaryType = B_SETTING;
          s.measureType = M_NONE;
          BOOST_FOREACH(Dma::sharedPointer dma, Dmas) {
            if (dma->isClosedBoundaryPipe(pipe) && !dma->isMeasuredBoundaryPipe(pipe)) {
              s.boundaryType = B_DMA_SETTING;
              break;
            }
          }
          summary.push_back(s);
        }
        if (pipe->doesHaveStatusParameter()) {
          ElementSummary s;
          s.element = element;
          s.data = pipe->statusParameter()->rootTimeSeries();
          s.boundaryType = B_STATUS;
          s.measureType = M_NONE;
          BOOST_FOREACH(Dma::sharedPointer dma, Dmas) {
            if (dma->isClosedBoundaryPipe(pipe) && !dma->isMeasuredBoundaryPipe(pipe)) {
              s.boundaryType = B_DMA_STATUS;
              break;
            }
          }
          summary.push_back(s);
        }
        break;
      }
        
      default:
        break;
    }
  }
  
  // add the timeseries gap statistics to the summary
  for(vector<ElementSummary>::iterator it = summary.begin();
      it != summary.end(); ++it) {
    TimeSeries::Stats gapstats;
    gapstats = (*it).data->gapsSummary(start, end);
    (*it).count = gapstats.count;
    (*it).minGap = gapstats.min;
    (*it).maxGap = gapstats.max;
    (*it).medianGap = gapstats.quartiles.q50;
  }
  
  return summary;
}
