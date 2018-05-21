var log = require('../../model/log.js');

export function get(req, res, next) {
  res.json(log.linkLogs());
}
