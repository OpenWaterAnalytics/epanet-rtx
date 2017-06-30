#include "OpcAdapter.h"

#include <open62541.h>

using namespace RTX;
using namespace std;


OpcAdapter::OpcAdapter(errCallback_t cb) : DbAdapter(cb) {
  _client = UA_Client_new(UA_ClientConfig_standard);
  
  
}


OpcAdapter::~OpcAdapter() {
  UA_Client_delete(_client);
}

const DbAdapter::adapterOptions OpcAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.supportsUnitsColumn = false;
  o.canAssignUnits = false;
  o.searchIteratively = false;
  o.supportsSinglyBoundQuery = false;
  o.implementationReadonly = true;
  
  return o;
}

string OpcAdapter::connectionString() {
  
}

void OpcAdapter::setConnectionString(const std::string &con) {
  
}

void OpcAdapter::doConnect() {
  _RTX_DB_SCOPED_LOCK;
  
  if (!_connected) {
    
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    
    UA_StatusCode retval = UA_Client_getEndpoints(_client, "opc.tcp://localhost:16664", &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
      UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
      stringstream msg;
      msg << retval;
      _errCallback(msg.str());
      return;
    }
    
    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t i=0;i<endpointArraySize;i++){
      printf("URL of endpoint %i is %.*s\n", (int)i,
             (int)endpointArray[i].endpointUrl.length,
             endpointArray[i].endpointUrl.data);
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    
    UA_Client_connect_username(_client, "opc.tcp://localhost:16664", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
      stringstream msg;
      msg << retval;
      _errCallback(msg.str());
      return;
    }
    
    
    
    
    _connected = true;
    
    
  }
  
  
  
}


IdentifierUnitsList OpcAdapter::idUnitsList() {
  _RTX_DB_SCOPED_LOCK;
  IdentifierUnitsList ids;
  
  
  printf("Browsing nodes in objects folder:\n");
  UA_BrowseRequest bReq;
  UA_BrowseRequest_init(&bReq);
  bReq.requestedMaxReferencesPerNode = 0;
  bReq.nodesToBrowse = UA_BrowseDescription_new();
  bReq.nodesToBrowseSize = 1;
  bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); /* browse objects folder */
  bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */
  UA_BrowseResponse bResp = UA_Client_Service_browse(_client, bReq);
  printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
  for (size_t i = 0; i < bResp.resultsSize; ++i) {
    for (size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
      UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
      if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        printf("%-9d %-16d %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
               ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
               ref->browseName.name.data, (int)ref->displayName.text.length,
               ref->displayName.text.data);
      } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        printf("%-9d %-16.*s %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
               (int)ref->nodeId.nodeId.identifier.string.length,
               ref->nodeId.nodeId.identifier.string.data,
               (int)ref->browseName.name.length, ref->browseName.name.data,
               (int)ref->displayName.text.length, ref->displayName.text.data);
      }
      /* TODO: distinguish further types */
      
      char nodeId[2560];
      sprintf(nodeId, "%.*s", (int)ref->displayName.text.length, ref->displayName.text.data);
      _nodes[string(nodeId)] = ref->nodeId.nodeId;
      
    }
  }
  UA_BrowseRequest_deleteMembers(&bReq);
  UA_BrowseResponse_deleteMembers(&bResp);
  
  return ids;
}


vector<Point> OpcAdapter::selectRange(const std::string &id, RTX::TimeRange range) {
  Point p;
  UA_NodeId thisNodeId = _nodes.at(id);
  
  UA_StatusCode retval;
  UA_Float value = 0;
  printf("\nReading the value of node (1, \"the.answer\"):\n");
  UA_Variant *val = UA_Variant_new();
  retval = UA_Client_readValueAttribute(_client, thisNodeId, val);
  if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) &&
     val->type == &UA_TYPES[UA_TYPES_FLOAT]) {
    value = *(UA_Float*)val->data;
    printf("the value is: %f\n", value);
    p = Point(time(NULL),(double)value);
  }
  UA_Variant_delete(val);
  
  return {p};
}





