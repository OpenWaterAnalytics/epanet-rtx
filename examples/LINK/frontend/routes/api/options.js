var proxy = require('../../model/linkProxy.js');

export function get(req, res, next) {
  proxy.get(req,res);
}

export function post(req, res, next) {
  proxy.post(req,res);
}
