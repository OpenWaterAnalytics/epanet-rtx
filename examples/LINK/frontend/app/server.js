const fs = require('fs-extra');
const app = require('express')();
const sapper = require('sapper');
const serve = require('serve-static');
var parseArgs = require('minimist');
var path = require('path');

var auth = require('../model/authentication.js');
var logger = require('../model/log.js');
var configuration = require('../model/configuration.js');
var spawnMonitor = require('../model/spawnMonitor.js');


import { routes } from './manifest/server.js';
import App from './App.html';
const { PORT = 3000 } = process.env;

// check runtime options using minimist
var debug = (process.env.NODE_ENV == 'development'); // using external LINK service = do not spawn
var argv = parseArgs(process.argv);
if (debug) {
  logger.logLine("====  DEBUG MODE: USING EXTERNAL LINK SERVICE ====");
  configuration.send(); // normally handled in spawn
} else {
	const linkExeName = 'link-server';
	const linkExePath = path.join(__dirname,'srv',process.platform,linkExeName);
	spawnMonitor.init(linkExePath);
}

require('../model/commonSetup.js').init(app);
app.use(serve('assets'));
auth.init(app);
app.use(auth.check, sapper({routes,App}));

app.listen(PORT, () => {
	console.log(`listening on port ${PORT}`);
});
