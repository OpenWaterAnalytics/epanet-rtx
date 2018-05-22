var proxy = require('../../model/linkProxy.js');
var config = require('../../model/configuration.js');
import fetch from 'node-fetch';

// get the configuration object from the LINK server,
// and save it locally in case of process termination

export function post(req,res,next) {
  console.log('SAVING CONFIG LOCALLY');
  fetch(proxy.host + '/config')
    .then(r => r.json())
    .then(r => {
      config.set(r);
      console.log('CONFIG SAVED');
      res.status(204).end();
    })
    .catch(err => {
      res.status(500).send({error: err});
    });
}
