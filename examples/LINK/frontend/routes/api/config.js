var proxy = require('../../model/linkProxy.js');
var config = require('../../model/configuration.js');
import fetch from 'node-fetch';
import _ from 'underscore';

export function get(req, res, next) {

  // /api/source/series?pretty=true --> download json file
  if (req.query.pretty && req.query.pretty === 'true') {
    return fetch('http://127.0.0.1:3131/config')
    .then(r => r.json())
    .then(r => {
      res.set('Content-Type', 'text/json');
      res.write(JSON.stringify(r, null, 2));
      res.end();
    });
  }
  else {
    // raw json
    proxy.get(req,res);
  }

};

export function post(req,res,next) {
  let newconfig = req.body;
  config.set(newconfig);
  console.log('posted config: ', req.body);
  proxy.post(req,res);
};
