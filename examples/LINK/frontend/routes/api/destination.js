var proxy = require('../_model/linkProxy.js');
import fetch from 'node-fetch';

export function get(req, res, next) {
  proxy.get(req,res);
}

export function post(req, res, next) {
  proxy.post(req,res);
}
