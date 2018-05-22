// linkProxy module, proxies get/post traffic to LINK service

var request = require('request');
const logger = require('./log.js');

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
  console.log(`PROXY GET ${req.path}`);
  request.get(url, (err,response,body) => {
    if (err) {
      logger.errLine("RTX-LINK Error :: ");
      logger.errLine(err);
      let msg = err.message;
      if (err.code == 'ECONNREFUSED') {
        msg = 'LINK service offline or unreachable';
      }
      res.status(500).json({error: `RTX-LINK Error :: ${msg} - Server status is unkown`});
    }
    else {
      logger.logLine(`GET ${url} --> ${response.statusCode}`);
      if (response.statusCode == 200 || response.statusCode == 204) {
        // ok
      }
      else {

      }
      res.status(response.statusCode).send(body);
    }
  });
}

module.exports.post = function(req, res) {
  var url = makeUrl(req.path);
  console.log(`PROXY POST ${req.path}`);
  var opts = {
    method: 'post',
    url: url,
    json: req.body
  };
  request(opts, function(err, response, body) {
    if (err) {
      logger.errLine('RTX-LINK SERVER Error :: ');
      logger.errLine(err);
      let msg = err.message;
      if (err.code == 'ECONNREFUSED') {
        msg = 'LINK service offline or unreachable';
      }
      res.status(500).json({error: `RTX-LINK Error :: ${msg} - Server status is unkown`});
    }
    else {
      logger.logLine('POST ' + url + ' --> ' + response.statusCode);
      res.status(response.statusCode).send(body);
    }
  }); // request

}

module.exports.host = link_server_host;
