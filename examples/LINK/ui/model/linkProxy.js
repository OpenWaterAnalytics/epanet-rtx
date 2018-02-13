// linkProxy module, proxies get/post traffic to LINK service

var request = require('request');
var CircularBuffer = require("circular-buffer");

const _maxLog = 1000;
const _logMessages = new CircularBuffer(_maxLog);

_logLine = function(msg){
  console.log(msg);
  _logMessages.enq(msg);
};
_errLine = function(msg){
  console.log(msg);
  _logMessages.enq(msg);
};

const link_server_host = 'http://127.0.0.1:3131';

function makeUrl(rawpath) {
  var path = rawpath;
  if (path.startsWith('/api/')) {
    path = link_server_host + path.substr(4);
  }
  return path;
}

module.exports.get = function(req, res) {
  var url = makeUrl(req.path);
  console.log(url);
  request.get(url, (err,response,body) => {
    if (err) {
      _errLine("RTX-LINK Error :: ");
      _errLine(err);
      let msg = err.message;
      if (err.code == 'ECONNREFUSED') {
        msg = 'LINK service offline or unreachable';
      }
      res.status(500).json({error: `RTX-LINK Error :: ${msg} - Server status is unkown`});
    }
    else {
      if (response.statusCode == 200 || response.statusCode == 204) {
        // ok
      }
      else {
        _logLine(`GET ${url} --> ${response.statusCode}`);
      }
      res.status(response.statusCode).send(body);
    }
  });
}

module.exports.post = function(req, res) {
  var url = makeUrl(req.path);
  var opts = {
    method: 'post',
    url: url,
    json: req.body
  };
  request(opts, function(err, response, body) {
    if (err) {
      _errLine('RTX-LINK SERVER Error :: ');
      _errLine(err);
      let msg = err.message;
      if (err.code == 'ECONNREFUSED') {
        msg = 'LINK service offline or unreachable';
      }
      res.status(500).json({error: `RTX-LINK Error :: ${msg} - Server status is unkown`});
    }
    else {
      _logLine('POST ' + url + ' --> ' + response.statusCode);
      res.status(response.statusCode).send(body);
    }
  }); // request

}
