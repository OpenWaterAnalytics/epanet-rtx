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
app.use(bodyParser.urlencoded({'extended':'true'}));            // parse application/x-www-form-urlencoded
app.use(bodyParser.json());                                     // parse application/json
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
    var login = clientReq.body.login;
    var auth = 'Basic ' + new Buffer(clientReq.body.login.user + ':' + clientReq.body.login.pass).toString('base64');
    request({
        method: 'POST',
        url: login.proto + '://' + login.url + '/api/dashboards/db',
        headers: {'Authorization': auth},
        json: clientReq.body.data
    }, function (error, response, body) {
        if(response.statusCode == 200){
            clientRes.status(200).json({message:"Dashboards Created."})
        } else {
            clientRes.status(500).json({message:"Failed to Create Dashboards. " + body.message})
        }
    });
});


// listen (start app with node server.js) ======================================
app.listen(8585);
console.log("App listening on port 8585");

