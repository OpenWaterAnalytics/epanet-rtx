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

// config options ===============================
const link_server_host = 'http://localhost:3131';
const linkExeName = 'link-server';
const linkExePath = path.join(__dirname,'srv',process.platform,linkExeName);
const linkDir = path.join(os.homedir(),'rtx_link');
const configFile = path.join(linkDir,'config.json');
const authFile = path.join(linkDir,'auth.json');
var _rtx_config = {};
var _link_auth = {};
jsf.spaces = 2;

backupConfig = function(){
  // to-do: send config.json to the cloud, for backup
};

// ensure that the config file is there. create it if it does not exist.
// these calls are blocking on purpose.
try {
  fs.ensureFileSync(configFile);
  _rtx_config = jsf.readFileSync(configFile);
} catch (e) {
  console.log(e);
}

try {
  fs.accessSync(authFile);
  _link_auth = jsf.readFileSync(authFile);
  console.log('found local auth: ');
  console.log(_link_auth);
} catch (e) {
  console.log('could not find local auth file. defaulting to admin // admin');
  _link_auth = {u:'admin',p:'admin'};
  fs.outputJsonSync(authFile, _link_auth, {}, function(err) {
    console.log('error writing auth file');
  });
}

sendLog = function(metric,field,value) {
  var opts = {
    method: "POST",
    url: link_server_host + "/log",
    json: {"metric":metric, "field":field, "value":value}
  };
  request(opts, function(error, response, body) {
    if (!error && response.statusCode == 200) {
      console.log('sent log entry');
      console.log(body);
    }
    else {
      console.log("ERROR sending log entry");
      console.log(error);
      console.log(body);
    }
  });
}


sendConfig = function() {
  var opts = {
    method: "POST",
    url: link_server_host + "/config",
    json: _rtx_config
  }; // opts
  request(opts, function(error, response, body) {
    if (error) {
      console.log("ERROR sending config :: ");
      console.log(error);
      return false;
    }
    else {
      console.log('sent config to LINK service');
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
  console.log("==================================================");
  console.log("====  DEBUG MODE: USING EXTERNAL LINK SERVICE ====");
  console.log("==================================================");
  sendConfig(); // normally handled in spawn
}


// start the LINK server cmd-line app ======================================
var link_monitor = respawn([linkExePath], {maxRestarts:10, sleep:2000});
link_monitor.on('stdout', function(data) {
  console.log('stdout: ' + data);
});
link_monitor.on('stderr', function(data) {
  console.log('stderr: ' + data);
});
link_monitor.on('spawn', function(child) {
  console.log('LINK MONITOR is spawning');
  console.log('SENDING CONFIG to new instance');
  setTimeout(function() {
    ok = sendConfig();
    if (!ok) {
      // respawn?
      console.log('error sending config. respawning');
      link_monitor.stop(function() {
          link_monitor.start();
      });
    }
  }, 2000);
});
link_monitor.on('warn', function(error) {
  console.log('LINK warning: ' + error);
});
link_monitor.on('exit', function(code,signal) {
  console.log('LINK exited with code: ' + code);
});
link_monitor.on('crash', function() {
  throw new Error('LINK SERVICE CRASHED');
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
  if (req.session && req.session.user === _link_auth.u && req.session.admin)
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
app.use(bodyParser.json());                                     // parse application/json
app.use(bodyParser.urlencoded({'extended':'true'}));            // parse application/x-www-form-urlencoded
app.use(methodOverride());


// application -------------------------------------------------------------
app.use('/', [auth, express.static(__dirname + '/public')]);

//
// relay dashboard configuration to grafana
//
app.post('/send_dashboards', function (clientReq, clientRes) {
  console.log('Sending Dashboards');
  var login = clientReq.body.login;
  var auth = 'Basic ' + new Buffer(clientReq.body.login.user + ':' + clientReq.body.login.pass).toString('base64');
  request({
        method: 'POST',
        url: login.proto + '://' + login.url + '/api/dashboards/db',
        headers: {'Authorization': auth},
        json: clientReq.body.data
    }, function (error, response, body) {
        if(response.statusCode == 200){
            clientRes.status(200).json({message:"Dashboard Created."})
        } else {
            clientRes.status(500).json({message:"Failed to Create Dashboard. " + body.message})
        }
    });
});

// handle getting the configuration data from NODE process
app.get('/config', function(req,res) {
  res.json(_rtx_config);
});

app.route('/relay/:linkPath/:subPath?') // odbc, units, source/series
.get(function(req,res) {
  var url = link_server_host + '/' + req.params.linkPath;
  if (req.params.subPath) { // re-assemble the subpath, if it exists.
    url = url.concat('/' + req.params.subPath);
  }
  request.get(url, function(error, response, body) {
    if (error) {
      console.log("RTX-LINK Error :: ");
      console.log(error);
      msg = error.message;
      if (error.code == 'ECONNREFUSED') {
        msg = 'LINK service offline or unreachable';
      }
      res.status(500).json({error: "RTX-LINK Error :: " + msg + " - Server status is " + link_monitor.status});
    }
    else {
      res.status(response.statusCode).send(body);
    }
  });
}); // get relay

// store configuration updates and relay them to LINK server
app.route('/config/:configPath')
.post(function(req,res) {
  console.log('posting config, with path ' + req.params.configPath);

  // save the config to local state
  const configKey = req.params.configPath;
  _rtx_config[configKey] = req.body;

  // save config to disk
  saveConfig(_rtx_config, function() {                // success
    console.log('SAVED CONFIG TO LOCAL STORE');
  }, function(e) {                                  // error
    console.log('RTX-LINK JSON WRITING ERROR :: ');
    console.log(e);
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
        console.log('RTX-LINK SERVER Error :: ');
        console.log(error);
        msg = error.message;
        if (error.code == 'ECONNREFUSED') {
          msg = 'LINK service offline or unreachable';
        }
        res.status(500).json({error: "RTX-LINK Error :: " + msg + " - Server status is " + link_monitor.status});
      }
      else {
        res.status(response.statusCode).send(body);
      }
    }); // request
  }
  else {
    res.status(200).json({message:'Saved Config.'})
  }

}); // POST


// listen (start app with node server.js) ======================================
app.listen(8585);
console.log("App listening on port 8585");


exports.link_app = app;
