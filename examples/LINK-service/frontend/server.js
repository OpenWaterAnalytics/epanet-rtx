// set up ========================
var express  = require('express');
var app      = express();                               // create our app w/ express
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

// config options ===============================
const link_server_host = 'http://localhost:3131';
const linkExeName = 'link-server';
const linkExePath = path.join(__dirname,'bin',process.platform,linkExeName);
const linkDir = path.join(os.homedir(),'rtx_link');
const configFile = path.join(linkDir,'config.json');
var config = {};
jsf.spaces = 2;

// ensure that the config file is there. create it if it does not exist
fs.ensureFile(configFile, function() {
  // ok, file is there
});


sendConfig = function() {
  var obj;
  try {
    config = jsf.readFileSync(configFile);
    var opts = {
        method: "POST",
        url: link_server_host + "/config",
        json: config
    }; // opts
    request(opts, function(error, response, body) {
        if (error) {
            console.log("ERROR sending config :: ");
            console.log(error);
            return false;
        }
        else {
            console.log('sent config to LINK service');
        }
    }); // request
  } catch (e) {
    console.log(e);
    return false;
  }
  return true;
};


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
if (!debug) {
  link_monitor.start() // spawn and watch
}








// express configuration =================
app.use(express.static(__dirname + '/public'));                 // set the static files location /public/img will be /img for users
app.use(bodyParser.json());                                     // parse application/json
app.use(bodyParser.urlencoded({'extended':'true'}));            // parse application/x-www-form-urlencoded
app.use(methodOverride());


// application -------------------------------------------------------------
app.get('/', function(req, res) {
    res.sendfile('./public/index.html'); // load the single view file (angular will handle the page changes on the front-end)
});


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


app.route('/dash')
.all(function (req,res,next) {
  if (req.method == 'POST') {
    config.dash = req.body;
    res.status(200).json({msg: 'OK'});
  }
  else if (req.method == 'GET') {
    res.json(config.dash);
  }
});

// proxy all GET/POST requests from here to LINK_service
app.route('/link-relay/:linkPath/:subPath?')
    .all(function (req, res, next) {
        if (!(req.method == 'POST' || req.method == 'GET')) {
            // only support get and post
            next();
            return;
        }
        var url = link_server_host + '/' + req.params.linkPath;
        if (req.params.subPath) { // re-assemble the subpath, if it exists.
            url = url.concat('/' + req.params.subPath);
        }
        var opts = {
            method: req.method,
            url: url,
            json: (req.method=="POST") ? req.body : false
        };

        request(opts, function(error, response, body) {
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
    }); // app.route.all







app.get('/saveConfig', function (req, res) {
  // get the config from LINK service, and save locally
  console.log('got command to save config');
  request({
        method: 'GET',
        url: link_server_host + '/config',
    }, function (error, response, body) {
      if (error) {
          console.log("RTX-LINK Error :: ");
          console.log(error);
          res.status(500).json({error: "RTX-LINK Error :: " + error.message});
      }
      else {
        // write the file
        // first, remember to save the dashboard data
        var dash = config.dash;
        config = JSON.parse(body);
        config.dash = dash;

        console.log(config);

        jsf.writeFile(configFile, config, function (err) {
            if (err) {
                res.status(500).json({error: 'ERROR: could not save configuration.'});
                console.log('could not save: ');
                console.log(err);
            } else {
                res.status(200).json({msg: "OK"});
                console.log('SAVED configuration');
            }
        });
      }
    });
});


// listen (start app with node server.js) ======================================
app.listen(8585);
console.log("App listening on port 8585");


exports.link_app = app;
