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

// configuration =================

app.use(express.static(__dirname + '/public'));                 // set the static files location /public/img will be /img for users
app.use(bodyParser.json());                                     // parse application/json
app.use(bodyParser.urlencoded({'extended':'true'}));            // parse application/x-www-form-urlencoded
app.use(methodOverride());


// application -------------------------------------------------------------
app.get('/', function(req, res) {
    res.sendfile('./public/index.html'); // load the single view file (angular will handle the page changes on the front-end)
});

//---  save/load configuration json file  ---//

var configFile = path.join(os.homedir(),'rtx_link','config.json');

jsf.spaces = 2;

app.post('/config', function (req, res) {
    jsf.writeFile(configFile, req.body, function (err) {
        if (err) {
            res.send('ERROR: could not save configuration.');
            console.log('could not save: ');
            console.log(err);
        } else {
            res.send('SAVED');
            console.log('SAVED configuration');
        }
    });
});

app.get('/config', function (req, res) {
    jsf.readFile(configFile, function (err, obj) {
        if (err) {
            console.log(err);
            res.status(404).json({message:'Could not find configuaration file'});
        }
        else {
            res.status(200).send(obj);
        }
    });
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


var link_server_host = 'http://localhost:3131';

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
                res.status(500).json({error: "RTX-LINK Error :: " + error.message});
            }
            else {
                res.status(response.statusCode).send(body);
            }
        });
    }); // app.route.all



// listen (start app with node server.js) ======================================
app.listen(8585);
console.log("App listening on port 8585");


var linkServer;
var launchLink = function () {
    // start up LINK service
    var linkExeName = 'link-server';
    var opts = [];
    var linkExePath = path.join('bin',process.platform,linkExeName);
    const child_process = require('child_process');

    linkServer = child_process.spawn(linkExePath, opts);

    linkServer.stdout.on('data', function (data) {
        console.log('stdout: ' + data);
    });

    linkServer.stderr.on('data', function (data) {
        console.log('stderr: ' + data);
    });

    linkServer.on('close', function (code) {
        console.log('child process exited with code ' + code);
    });

    linkServer.on('error', function (err) {
        console.log('could not launch LINK: ' + err);
    });

};



launchLink();
