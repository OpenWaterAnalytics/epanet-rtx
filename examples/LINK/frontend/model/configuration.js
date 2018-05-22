const _ = require('underscore');
const fs = require('fs-extra');
var path = require('path');
var os = require('os');
var jsf = require('jsonfile');
const log = require('./log.js');
const proxy = require('./linkProxy.js');
const fetch = require('node-fetch');

const linkDir = path.join(os.homedir(),'rtx_link');
const configFile = path.join(linkDir,'config.json');
var _rtx_config = {};
jsf.spaces = 2;

// ensure that the config file is there. create it if it does not exist.
// these calls are blocking on purpose.
try {
  if (!fs.pathExistsSync(configFile)) {
    fs.ensureFileSync(configFile);
    jsf.writeFileSync(configFile, {});
  }
  _rtx_config = jsf.readFileSync(configFile);
} catch (e) {
  log.errLine(e);
}

var defaultConfig = {
  source:      {_class: 'odbc', name: '', connectionString: ''},
  destination: {_class: 'influx', name: '', connectionString: ''},
  options:     { window:12, interval:5, backfill:30, smart:true},
  series:      [],
  dash:        { proto:'http' },
  run:         { run: false },
  analytics:   [ ]
};
_rtx_config = _.defaults(_rtx_config,defaultConfig);


module.exports.config = _rtx_config;

module.exports.set = function(newconfig, successCallback, errorCallback) {
  _rtx_config = newconfig;
  jsf.writeFile(configFile, _rtx_config, {spaces: 2}, (err) => {
    if (err) {
      log.errLine(err);
      typeof errorCallback == "function" && errorCallback(err);
    }
  });
}

module.exports.send = () => {
  fetch(proxy.host + '/config', {
    method:'POST',
    body:JSON.stringify(_rtx_config),
    headers: { 'Content-Type': 'application/json' }
  })
  .then(r => {
    console.log('configuration sent');
  })
  .catch(err => {
    console.log('send failed:');
    log.errLine(err.message);
  });
}
