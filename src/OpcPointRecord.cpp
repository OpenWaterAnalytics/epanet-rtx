#include "OpcPointRecord.h"
#include <time.h>

#include <ua_client.h>
#include <ua_types.h>
#include <ua_server.h>
#include <logger_stdout.h>
#include <networklayer_tcp.h>
#include <ua_config_standard.h>

using namespace std;
using namespace RTX;


OpcPointRecord::OpcPointRecord() {
  // init
  _client = UA_Client_new(UA_ClientConfig_standard);
  _connected = false;
}

void OpcPointRecord::setConnectionString(const std::string &string) {
  _endpoint = string;
}

string OpcPointRecord::connectionString() {
  return _endpoint;
}
/*
void _getChildren(OpcUa::Node node, vector<string>& list, int level = 0);
void _getChildren(OpcUa::Node node, vector<string>& list, int level) {
  
  for (int i = 0; i < level; ++i) {
    cout << "  ";
  }
  
  string browseName = node.GetBrowseName().Name;
  
  string identifier = "";
  if (node.GetId().IsString()) {
    identifier = node.GetId().GetStringIdentifier();
    
    list.push_back(identifier);
    
  }
  
  cout << browseName << " :: " << identifier << " == ";
  
  OpcUa::Variant type = node.GetDataType();
  try {
    OpcUa::Variant value = node.GetValue();
    if (value.Type() == OpcUa::VariantType::FLOAT) {
      cout << "FLOAT: " << value.As<float>();
    }
    else if (value.Type() == OpcUa::VariantType::DOUBLE) {
      cout << "DOUBLE: " << value.As<double>();
    }
    else if (value.Type() == OpcUa::VariantType::BOOLEAN) {
      cout << "BOOL: " << value.As<bool>();
    }
    else if (value.Type() == OpcUa::VariantType::INT16) {
      cout << "INT16: " << value.As<int16_t>();
    }
    else {
      cout << "???????: " << value.ToString();
    }
    
  }
  catch(const std::exception &e) {
    cerr << e.what();
  }
  
  //OpcUa::DataValue v = node.GetDataValue();
    
  cout << "";
  
  
  cout << endl;
  
//  
//  auto val = node.GetDataValue();
//  cout << " == " << val.Value.ToString() << endl;
  
  if (node.GetChildren().size() > 0) {
    for (auto child : node.GetChildren()) {
      _getChildren(child, list, level+1);
    }
  }
  
}
*/

bool _isNumeric(OpcUa::Variant v);
bool _isNumeric(OpcUa::Variant v) {
  OpcUa::VariantType t = v.Type();
  
  return (   t == OpcUa::VariantType::BOOLEAN
          || t == OpcUa::VariantType::DOUBLE
          || t == OpcUa::VariantType::FLOAT
          || t == OpcUa::VariantType::INT16
          || t == OpcUa::VariantType::INT32
          || t == OpcUa::VariantType::INT64
          || t == OpcUa::VariantType::UINT16
          || t == OpcUa::VariantType::UINT32
          || t == OpcUa::VariantType::UINT64);
  
}

Point _toPoint(OpcUa::DataValue& v);
Point _toPoint(OpcUa::DataValue& v) {
  
  Point p;
  if (_isNumeric(v.Value)) {
    OpcUa::DateTime dt = v.SourceTimestamp;
    time_t t;
    if (dt.Value > 11644473600) {
      t = OpcUa::DateTime::ToTimeT(dt);
    }
    else {
      t = time(NULL);
    }
    double value = 0;
    Point::PointQuality q = v.Status == OpcUa::StatusCode::Good ? Point::opc_good : Point::opc_bad;
    
    
    switch (v.Value.Type()) {
      case OpcUa::VariantType::DOUBLE:
        value = v.Value.As<double>();
        break;
      case OpcUa::VariantType::BOOLEAN:
        value = (double)v.Value.As<bool>();
        break;
      case OpcUa::VariantType::FLOAT:
        value = (double)v.Value.As<float>();
        break;
      case OpcUa::VariantType::INT16:
        value = (double)v.Value.As<int16_t>();
        break;
      case OpcUa::VariantType::INT32:
        value = (double)v.Value.As<int32_t>();
        break;
      case OpcUa::VariantType::INT64:
        value = (double)v.Value.As<int64_t>();
        break;
      default:
        value = stod(v.Value.ToString()); // 💩
        break;
    }
    
    p = Point(t,value,q);
    
  }
  return p;
}



void _getChildren(OpcUa::Node node, map<string,OpcUa::Node>& nodes, int level = 0);
void _getChildren(OpcUa::Node node, map<string,OpcUa::Node>& nodes, int level) {
  if (node.GetId().IsString() && _isNumeric(node.GetValue())) {
    string identifier = node.GetId().GetStringIdentifier();
    OpcUa::QualifiedName qn = node.GetBrowseName();
    string browse = qn.Name;
    nodes[identifier] = node;
    cout << node << endl;
  }
  if (node.GetChildren().size() > 0) {
    for (auto child : node.GetChildren()) {
      _getChildren(child, nodes, level+1);
    }
  }
}


bool OpcPointRecord::isConnected() {
  return _connected;
}

void OpcPointRecord::dbConnect() throw(RtxException) {
  
  try {
    _client.Connect(_endpoint);
    _connected = true;
  }
  catch(const exception &e) {
    errorMessage = string(e.what());
    return;
  }
  
  for (auto endpoint : _client.GetServerEndpoints()) {
    cout << "ENDPOINT: " << endpoint.EndpointUrl << endl;
  }
  
  
  OpcUa::Node root = _client.GetRootNode();
  std::cout << "Root node is: " << root << std::endl;
  
  
  //get a node from standard namespace using objectId
  std::cout << "NamespaceArray is: " << std::endl;
  OpcUa::Node nsnode = _client.GetNode(OpcUa::ObjectId::Server_NamespaceArray);
  OpcUa::Variant ns = nsnode.GetValue();
  
  for (std::string d : ns.As<std::vector<std::string>>()) {
    std::cout << "    " << d << std::endl;
  }
  
  
}

const map<string, Units> OpcPointRecord::identifiersAndUnits() {
  
  map<string, Units> ids;
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (!isConnected()) {
    return ids;
  }
  
  OpcUa::Node root = _client.GetRootNode();
  
  OpcUa::Node objectsNode = _client.GetObjectsNode();
  _getChildren(objectsNode, _nodes);
  
  for (auto item : _nodes) {
    ids[item.first] = RTX_NO_UNITS;
  }
  
  _identifiersAndUnitsCache = ids;
  
  
  return ids;
}





vector<Point> OpcPointRecord::selectRange(const string& id, TimeRange range) {
  
  vector<Point> points;
  
  OpcUa::Node root = _client.GetRootNode();
  
  if (_nodes.count(id) > 0) {
    OpcUa::Node selectNode = _nodes.at(id);
    
    OpcUa::DataValue v = selectNode.GetDataValue();
    Point p = _toPoint(v);
    
    
    points = {p};
    
  }
  
  
  return points;
}










