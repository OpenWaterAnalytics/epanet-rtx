var proxy = require('../../../model/linkProxy.js');
import fetch from 'node-fetch';

export function get(req, res, next) {

  // /api/source/series?format=csv --> download csv file
  if (req.query.format && req.query.format === 'csv') {
    fetch('http://127.0.0.1:3131/source/series')
    .then(r => r.json())
    .then(r => {
      res.set('Content-Type', 'text/csv');
      for (let series of r) {
        res.write(`${series.name}, ${series.units.unitString}, ${series.name}, ${series.units.unitString}\n`);
      }
      res.end();
    })
    .catch(err => {
      res.write(`${err}`);
      res.end();
    });
    return;
  }
  else {
    // relay to LINK
    proxy.get(req,res);
  }


}

export function post(req, res, next) {
  proxy.post(req,res);
}
