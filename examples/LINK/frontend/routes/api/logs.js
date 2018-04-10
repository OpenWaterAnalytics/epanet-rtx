var proxy = require('../_model/linkProxy.js');

export function get(req, res, next) {
  res.json(proxy.linkLogs());
}
