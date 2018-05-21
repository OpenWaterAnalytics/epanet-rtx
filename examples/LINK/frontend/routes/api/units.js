var proxy = require('../../model/linkProxy.js');
export function get(req, res, next) {
  proxy.get(req,res);
}
