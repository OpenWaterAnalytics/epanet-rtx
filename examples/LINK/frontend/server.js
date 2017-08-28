// set up ========================
var express  = require('express');
var app      = express();
var session = require('express-session');         // create our app w/ express, add session
var bodyParser = require('body-parser');    // pull information from HTML POST (express4)
var methodOverride = require('method-override'); // simulate DELETE and PUT (express4)
var jsf = require('jsonfile');
var http = require('http');
var request = require('request');
var path = require('path');
var os = require('os');
var fs = require('fs-extra');
var respawn = require('respawn');
var parseArgs = require('minimist');
var ip = require('ip');
var CircularBuffer = require("circular-buffer");

// config options ===============================
const link_server_host = 'http://127.0.0.1:3131';
const linkExeName = 'link-server';
const linkExePath = path.join(__dirname,'srv',process.platform,linkExeName);
const linkDir = path.join(os.homedir(),'rtx_link');
const configFile = path.join(linkDir,'config.json');
const authFile = path.join(linkDir,'auth.json');
var _rtx_config = {};
var _link_auth = {};
jsf.spaces = 2;



const _maxLog = 1000;
const _logMessages = new CircularBuffer(_maxLog);

_logLine = function(msg){
  console.log(msg);
  _logMessages.enq(msg);
};
_errLine = function(msg){
  console.log(msg);
  _logMessages.enq(msg);
};



backupConfig = function(){
  // to-do: send config.json to the cloud, for backup
};

// ensure that the config file is there. create it if it does not exist.
// these calls are blocking on purpose.
try {
  if (!fs.pathExistsSync(configFile)) {
    fs.ensureFileSync(configFile);
    jsf.writeFileSync(configFile, {run:false});
  }
  _rtx_config = jsf.readFileSync(configFile);
} catch (e) {
  _errLine(e);
}

try {
  if (!fs.pathExistsSync(authFile)) {
    fs.ensureFileSync(authFile);
    jsf.writeFileSync(authFile,{u:'admin',p:'admin'});
  }
  _link_auth = jsf.readFileSync(authFile);
  _logLine('found local auth: ');
  _logLine(JSON.stringify(_link_auth));
} catch (e) {
  _errLine("could not find or create auth file");
  _errLine(e);
}

sendLog = function(metric,field,value) {
  var opts = {
    method: "POST",
    url: link_server_host + "/log",
    json: {"metric":metric, "field":field, "value":value}
  };
  request(opts, function(error, response, body) {
    if (!error && response.statusCode == 200) {
      _logLine('sent log entry');
      _logLine(body);
    }
    else {
      _errLine("ERROR sending log entry");
      _errLine(error);
      _errLine(body);
    }
  });
}


sendConfig = function() {
  var opts = {
    method: "POST",
    url: link_server_host + "/config",
    json: _rtx_config
  }; // opts
  _logLine("SENDING CONFIG:");
  console.log(opts);
  request(opts, function(error, response, body) {
    if (error) {
      _errLine("ERROR sending config :: ");
      _errLine(error);
      return false;
    }
    else {
      _logLine('sent config to LINK service');
      sendLog('startinfo','ip',ip.address());
    }
  }); // request
  return true;
};

saveConfig = function(config,successCallback,errorCallback) {
  try {
    console.log(config);
    jsf.writeFileSync(configFile, config);
  }
  catch (e) {
    typeof errorCallback == "function" && errorCallback(e);
    return;
  }
  typeof successCallback == "function" && successCallback();
}


// check runtime options using minimist
var debug = false; // using external LINK service = do not spawn
var argv = parseArgs(process.argv);
if (argv.debug) {
  debug = true;
  _logLine("==================================================");
  _logLine("====  DEBUG MODE: USING EXTERNAL LINK SERVICE ====");
  _logLine("==================================================");
  sendConfig(); // normally handled in spawn
}


// start the LINK server cmd-line app ======================================
var link_monitor = respawn([linkExePath], {maxRestarts:10, sleep:2000});
link_monitor.on('stdout', function(data) {
  _logLine('stdout: ' + data);
});
link_monitor.on('stderr', function(data) {
  _errLine('stderr: ' + data);
});
link_monitor.on('spawn', function(child) {
  _logLine('LINK MONITOR is spawning');
  _logLine('SENDING CONFIG to new instance');
  setTimeout(function() {
    ok = sendConfig();
    if (!ok) {
      // respawn?
      _errLine('error sending config. respawning');
      link_monitor.stop(function() {
          link_monitor.start();
      });
    }
  }, 2000);
});
link_monitor.on('warn', function(error) {
  _logLine('LINK warning: ' + error);
});
link_monitor.on('exit', function(code,signal) {
  _logLine('LINK exited with code: ' + code);
});
link_monitor.on('crash', function() {
  throw new Error('SERVICE MONITOR HAS CRASHED');
})
if (!debug) {
  link_monitor.start() // spawn and watch
}


app.get('/login-page', function(req,res) {
  res.sendFile(__dirname + '/public/login-page.html'); // login page
});

// auth options ===============================
app.use(session({
    secret: 'status-imposing-abel',
    resave: true,
    saveUninitialized: true
}));
// Authentication and Authorization Middleware
var auth = function(req, res, next) {
  if ((req.session && req.session.user === _link_auth.u && req.session.admin)
      || req.url.includes('/assets/') /* allow access to js/css assets */ )
    return next();
  else
    return res.redirect('/login-page');
};
app.get('/login', function (req, res) {
  if(req.query.u === _link_auth.u && req.query.p === _link_auth.p) {
    req.session.user = _link_auth.u;
    req.session.admin = true;
    res.redirect('/');
  }
  else {
    req.session.destroy();
    res.send("login failed");
  }
});
// Logout endpoint
app.get('/logout', function (req, res) {
  req.session.destroy();
  res.send("logout success!");
});

// express configuration =================
//app.use(express.static(__dirname + '/public'));                 // set the static files location /public/img will be /img for users
app.use(bodyParser.json({limit: '10mb'}));                                     // parse application/json
app.use(bodyParser.urlencoded({limit: '10mb', extended: true}));            // parse application/x-www-form-urlencoded
app.use(methodOverride());




//
// relay dashboard configuration to grafana
//
app.post('/send_dashboards', function (clientReq, clientRes) {
  _logLine('Sending Dashboards');
  var login = clientReq.body.login;
  var auth = 'Basic ' + new Buffer(clientReq.body.login.user + ':' + clientReq.body.login.pass).toString('base64');
  request({
        method: 'POST',
        url: login.proto + '://' + login.url + '/api/dashboards/db',
        headers: {'Authorization': auth},
        json: clientReq.body.data
    }, function (error, response, body) {
      if (error) {
        clientRes.status(500).json({message:"Could not connect to server."});
        _errLine(error);
      }
      else if (response.statusCode == 200){
        clientRes.status(200).json({message:"Dashboard Created."});
      } else {
        clientRes.status(500).json({message:"Failed to Create Dashboard. " + body.message});
      }
    });
});

// handle getting the configuration data from NODE process
app.get('/config', function(req,res) {
  res.json(_rtx_config);
});

// get log messages
app.get('/svc/logs', function(req,res) {
  console.log('getting log rows. ', _logMessages.size(), ' messages in queue');
  res.json(_logMessages.toarray());
});

app.route('/relay/:linkPath/:subPath?') // odbc, units, source/series
.get(function(req,res) {
  var url = link_server_host + '/' + req.params.linkPath;
  if (req.params.subPath) { // re-assemble the subpath, if it exists.
    url = url.concat('/' + req.params.subPath);
  }
  request.get(url, function(error, response, body) {
    if (error) {
      _errLine("RTX-LINK Error :: ");
      _errLine(error);
      msg = error.message;
      if (error.code == 'ECONNREFUSED') {
        msg = 'LINK service offline or unreachable';
      }
      res.status(500).json({error: "RTX-LINK Error :: " + msg + " - Server status is " + link_monitor.status});
    }
    else {
      if (response.statusCode == 200 || response.statusCode == 204) {

      }
      else {
        _logLine('GET ' + url + ' --> ' + response.statusCode);
      }
      res.status(response.statusCode).send(body);
    }
  });
}); // get relay

// store configuration updates and relay them to LINK server
app.route('/config/:configPath')
.post(function(req,res) {
  _logLine('posting config, with path ' + req.params.configPath);

  // save the config to local state
  const configKey = req.params.configPath;
  _rtx_config[configKey] = req.body;

  // save config to disk
  saveConfig(_rtx_config, function() {                // success
    _logLine('SAVED CONFIG TO LOCAL STORE');
  }, function(e) {                                  // error
    _errLine('RTX-LINK JSON WRITING ERROR :: ');
    _errLine(e);
    res.status(500).json({error: "RTX-LINK NODE Error :: " + e.message});
  });


  // check that the key is supported by LINK server:
  const supportedPaths = ['source','destination','options','series','run','analytics'];
  if (supportedPaths.indexOf(configKey) !== -1) {
    // finally relay the config subpath to LINK server
    var url = link_server_host + '/' + req.params.configPath;
    var opts = {
      method: req.method,
      url: url,
      json: req.body
    };
    request(opts, function(error, response, body) {
      if (error) {
        _errLine('RTX-LINK SERVER Error :: ');
        _errLine(error);
        msg = error.message;
        if (error.code == 'ECONNREFUSED') {
          msg = 'LINK service offline or unreachable';
        }
        res.status(500).json({error: "RTX-LINK Error :: " + msg + " - Server status is " + link_monitor.status});
      }
      else {
        _logLine('POST ' + configKey + ' --> ' + response.statusCode);
        res.status(response.statusCode).send(body);
      }
    }); // request
  }
  else {
    res.status(200).json({message:'Saved Config.'})
  }

}); // POST

// application -------------------------------------------------------------
var myRoutes = ['main','source','destination','analytics','options','run','dashboard','about','logs'];
for (var i = 0, len = myRoutes.length; i < len; i++) {
  var r = myRoutes[i];
  var theRoute = '/' + r;
  console.log("adding route: " + theRoute);
  app.use(theRoute, [auth, express.static(__dirname + '/public/index.html')]);
}
app.use('/', [auth, express.static(__dirname + '/public')]);

// listen (start app with node server.js) ======================================
var server = app.listen(8585);
_logLine("App listening on port 8585");



var gracefulShutdown = function() {
  _logLine("Received kill signal, shutting down gracefully.");
  link_monitor.stop();
  server.close(function() {
    _logLine("Closed out remaining connections.");
    process.exit();
  });

   // if after
   setTimeout(function() {
       console.error("Could not close connections in time, forcefully shutting down");
       process.exit();
  }, 5*1000);
}

// listen for TERM signal .e.g. kill
process.on ('SIGTERM', gracefulShutdown);
// listen for INT signal e.g. Ctrl-C
process.on ('SIGINT', gracefulShutdown);





exports.link_app = app;
