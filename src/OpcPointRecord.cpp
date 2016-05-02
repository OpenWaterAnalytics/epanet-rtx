#include "OpcPointRecord.h"


using namespace std;
using namespace RTX;


OpcPointRecord::OpcPointRecord() {
  // init
}

void OpcPointRecord::setConnectionString(const std::string &string) {
  _endpoint = string;
}

string OpcPointRecord::connectionString() {
  return _endpoint;
}

void _printChildren(OpcUa::Node node, int level = 0);
void _printChildren(OpcUa::Node node, int level) {
  
  for (int i = 0; i < level; ++i) {
    cout << "  ";
  }
  cout << node.GetValue().ToString() << endl;
//  
//  auto val = node.GetDataValue();
//  cout << " == " << val.Value.ToString() << endl;
  
  if (node.GetChildren().size() > 0) {
    for (auto child : node.GetChildren()) {
      _printChildren(child, level+1);
    }
  }
  
}

void OpcPointRecord::dbConnect() throw(RtxException) {
  
  _client.Connect(_endpoint);
  
  for (auto endpoint : _client.GetServerEndpoints()) {
    cout << "ENDPOINT: " << endpoint.EndpointUrl << endl;
  }
  
  
  OpcUa::Node root = _client.GetRootNode();
  std::cout << "Root node is: " << root << std::endl;
  
  //get and browse Objects node
  std::cout << "Child of objects node are: " << std::endl;
  OpcUa::Node objectsNode = _client.GetObjectsNode();
  _printChildren(objectsNode);
  
  //get a node from standard namespace using objectId
  std::cout << "NamespaceArray is: " << std::endl;
  OpcUa::Node nsnode = _client.GetNode(OpcUa::ObjectId::Server_NamespaceArray);
  OpcUa::Variant ns = nsnode.GetValue();
  
  for (std::string d : ns.As<std::vector<std::string>>()) {
    std::cout << "    " << d << std::endl;
  }
  
  
  
  
}

const map<string, Units> OpcPointRecord::identifiersAndUnits() {
  
  
  
}