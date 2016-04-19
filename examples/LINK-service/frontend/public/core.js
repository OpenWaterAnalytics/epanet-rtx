// public/core.js
var rtxLink = angular.module('rtxLink', ['ngRoute'])

.config(['$httpProvider', function($httpProvider) {
    $httpProvider.defaults.useXDomain = true;
    delete $httpProvider.defaults.headers.common['X-Requested-With'];
}
])

.config(['$locationProvider', function ($locationProvider) {
    $locationProvider
        .html5Mode(false)
        .hashPrefix('');
}])

.run(function ($rootScope, $timeout, $http) {

    $rootScope.config = {
        source:      {},
        destination: {},
        fetch:       {},
        series:      [],
        dash:        { proto:'http' }
    };

    $rootScope.sourceTypes = {
        'odbc': [
            {
                key:'name',
                text:'Name',
                placeholder:'SCADA User-Defined Name',
                inputType:'text-line'
            },{
                key:'driver',
                text:'ODBC Driver',
                placeholder:'MS-SQL ODBC Provider',
                inputType:'select-line'
            },{
                key:'connectionString',
                text:'Connection',
                placeholder:'SVR=192.168.0.1;blah=blah',
                inputType:'text-line'
            },{
                key:'meta',
                text: 'Name lookup query',
                placeholder: 'SELECT tagname, units FROM tag_list ORDER BY tagname ASC',
                inputType: 'text-line'
            },{
                key: 'range',
                text: 'Data lookup query',
                placeholder: 'SELECT date, value, quality FROM tbl WHERE tagname = ? AND date >= ? AND date <= ? ORDER BY date ASC',
                inputType: 'text-line'
            }
        ],
        'sqlite': [
            {
                key:'name',
                text:'Name',
                placeholder:'User-defined Name',
                inputType:'text-line'
            },
            {
                key:'connectionString',
                text:'File Name',
                placeholder:'tempfile.sqlite',
                inputType:'text-line'
            }
        ],
        'influx': [
            {
                key:'name',
                text:'Name',
                placeholder:'User-defined Name',
                inputType:'text-line'
            },
            {
                key:'connectionString',
                text:'Connection',
                placeholder:'host=http://flux.citilogics.io&u=user&p=pass',
                inputType:'text-line'
            }
        ]
    };

    $rootScope.rtxTypes = {
        'odbc': 'ODBC',
        'sqlite': 'SQLite',
        'influx': 'Influx'
    };


    $rootScope.errorMessagePromise = $timeout();
    $rootScope.infoMessagePromise = $timeout();

    $rootScope.showError = function (msg) {
        $timeout.cancel($rootScope.errorMessagePromise);
        $rootScope.errorMessage = msg;
        $rootScope.errorMessagePromise = $timeout(function () {
            $rootScope.errorMessage = '';
        }, 5000);
    };

    $rootScope.showInfo = function (msg) {
        $timeout.cancel($rootScope.infoMessagePromise);
        $rootScope.infoMessage = msg;
        $rootScope.infoMessagePromise = $timeout(function () {
            $rootScope.infoMessage = '';
        }, 5000);
    };

    $rootScope.connectRecord = function (data, endpoint, callback) {
        $http({
            method: 'POST',
            url: 'http://localhost:3131/' + endpoint,
            headers: { 'Content-Type': undefined }, // undefined content-type to overcome CORS
            data: data
        }).then(function (response) {
            $rootScope.showInfo('Connection OK');
            typeof callback == "function" && callback();
        }, function (response) {
            var errStr = "";
            if (response.data) {
                errStr = response.data['error'];
            } else {
                errStr = 'LINK Service Not Responding';
            }
            $rootScope.showError('Error Connecting: ' + errStr);
        });
    };

    $rootScope.notifyHttpError = function (response) {
        if (response.data) {
            if (response.data.error) {
                errStr = response.data.error;
            }
            else if (response.data.message) {
                errStr = response.data.message;
            }
            else {
                errStr = "Unknown Error";
            }
        } else {
            errStr = 'LINK Service Not Responding';
        }
        $rootScope.showError('Error Connecting: ' + errStr);
    }

    $rootScope.saveConfig = function () {
        // get config from LINK Server, send to Node Server.
        // Node will save the config locally.
        $http.get('http://localhost:3131/config')
            .then(function (response) {
                if (response.data) {
                    var configData = response.data;
                    configData.dash = $rootScope.config.dash;
                    $http({
                        method: 'POST',
                        url: 'http://localhost:8585/config',
                        headers: {'Content-type': 'application/json'},
                        data: configData
                    }).then(function (postResponse) {
                        console.log('configuration saved via node: ');
                        console.log(response.data);
                        $rootScope.showInfo('Saved Complete Configuration');
                    }, function (errResponse) {
                        console.log('configuration not saved');
                        $rootScope.notifyHttpError('Configuration Not Saved');
                    });
                }
            }, function (response) {
                $rootScope.notifyHttpError(response);
            });
    };
    
    
    $rootScope.initializeConfig = function () {
        // get the stored configuration from Node, and
        // send that to LINK to initialize.
        $http.get('http://localhost:8585/config')
            .then(function (res) {
                // success
                var data = res.data;
                console.log('retrieved configuration data:');
                console.log(data);
                console.log('sending to RTX-LINK');

                $rootScope.config = data;

                $http({
                    method: 'POST',
                    url: 'http://localhost:3131/config',
                    headers: { 'Content-Type': undefined }, // undefined content-type to overcome CORS
                    data: data
                }).then(function (res) {
                    // success
                    console.log('Successfully sent configuration to RTX-LINK.');
                    console.log(res);
                }, function (res) {
                    // error
                    $rootScope.notifyHttpError(res);
                });

            }, function (res) {
                // error
                $rootScope.notifyHttpError(res);
            });
    };


    $rootScope.getFormData = function ($scope,urlPath,callback) {
        $http.get('http://localhost:3131/' + urlPath)
            .then(function (response) {
                typeof callback == "function" && callback(response.data);
            }, function (response) {
                $rootScope.notifyHttpError(response);
            });
    };






    // ON LOAD
    $rootScope.initializeConfig();
    
})

.controller('HeaderController', function HeaderController($scope, $location) {
    $scope.links = [
        //{url:'main', text:'Main'},
        {url:'source', text:'Source'},
        {url:'destination', text:'Destination'},
        {url:'options', text:'Options'},
        {url:'run', text:'Run'},
        {url:'dashboard', text:'Dashboard'},
        {url:'about', text:'About'}
    ];

    $scope.isActive = function (viewLocation) {
        return viewLocation === $location.path();
    };
})

.config(['$routeProvider', function ($routeProvider) {
    $routeProvider
        .when('/main', {
            templateUrl: 'templates/main.part.html',
            controller: 'MainController'
        })
        .when('/source', {
            templateUrl: 'templates/source.part.html',
            controller: 'SourceController'
        })
        .when('/about', {
            templateUrl: 'templates/about.part.html',
            controller: 'AboutController'
        })
        .when('/destination', {
            templateUrl: 'templates/destination.part.html',
            controller: 'DestinationController'
        })
        .when('/options', {
            templateUrl: 'templates/options.part.html',
            controller: 'OptionsController'
        })
        .when('/run', {
            templateUrl: 'templates/run.part.html',
            controller: 'RunController'
        })
        .otherwise({
            redirectTo: '/main'
        });
}])

.controller('OdbcDriverSelectController', function OdbcDriverSelectController($scope, $http) {
    $scope.drivers = ["getting driver list..."];

    $http.get('http://localhost:3131/odbc')
        .then(function (response) {
            $scope.drivers = response.data;
        }, function (response) {
            $scope.drivers = ["failed to get driver list"];
        });

})

.controller('MainController', function MainController($scope, $http, $interval) {

})


/**************************/
/******** SOURCE **********/
/**************************/

.controller('SourceController', function SourceController($rootScope, $scope, $http, $interval, $location) {
    $scope.availableUnits = [];
    $rootScope.config.source = {_class: 'none'};
    $scope.sourceSeries = [];
    $scope.statusMsg = '';
    $scope.showTsList = false;



    $scope.saveAndNext = function () {
        var tsList = $scope.sourceSeries.filter(function (v) { return v._link_selected; });
        $rootScope.config.series = tsList;
        $http({
            method: 'POST',
            url: 'http://localhost:3131/series',
            headers: { 'Content-Type': undefined }, // undefined content-type to overcome CORS
            data: tsList
        }).then(function (response) {
            $location.path('/destination');
        }, function (response) {
            var errStr = response.data
                ? response.data['error']
                : 'LINK Service Not Responding';
            $rootScope.showError('Error Saving Options: ' + errStr);
        });
    };

    // get units list
    var getUnits = function () {
        $http.get('http://localhost:3131/units')
            .then(function (response) {
                $scope.availableUnits = response.data;
            }, function (response) {
                // failed
            });
    };

    $scope.isType = function (typeName) {
        return typeName === $rootScope.config.source._class;
    };
    $scope.setSourceType = function (typeName) {
        $rootScope.config.source = {_class: typeName};
        $scope.sourceSeries = [];
        $scope.showTsList = false;
    };

    $scope.connect = function () {
        $rootScope.connectRecord($rootScope.config.source, 'source', function () {
            $scope.refreshSeriesList();
        });
    };

    $scope.refreshSeriesList = function () {
        $rootScope.showInfo('Connected. Fetching List of Series...')
        getUnits();
        $http.get('http://localhost:3131/source/series')
            .then(function (response) {
                $scope.sourceSeries = response.data;
                angular.forEach($scope.sourceSeries, function (series) {
                    series['_link_selected'] = false;
                });
                $rootScope.showInfo('Found ' + $scope.sourceSeries.length + ' Series');
                $scope.showTsList = true;
                $scope.reconcileSeries();
            }, function (response) {
                $rootScope.notifyHttpError(response);
            });
    };

    $scope.save = function () {
        console.log("form data: " + $rootScope.config.source);
    };

    $scope.removeSeries = function (selected) {
        $scope.sourceSeries = $scope.sourceSeries.filter(function (v) {
            return v._link_selected != selected;
        });
    };

    $scope.reconcileSeries = function () {
        // get the list of dupe series from LINK
        // and match them to the list of series (if any)
        // in the searchable list, and make sure they are selected.
        $http.get('http://localhost:3131/series')
            .then(function (response) {
                var dupeSeries = response.data;
                var dupeNames = [];
                angular.forEach(dupeSeries, function (series) {
                    dupeNames.push(series['name']);
                });
                angular.forEach($scope.sourceSeries, function (series) {
                    var thisName = series['name'];
                    if (-1 !== dupeNames.indexOf(thisName)) {
                        series['_link_selected'] = true;
                    }
                    else {
                        series['_link_selected'] = false;
                    }
                });

            }, function (errResponse) {

            });
    };


    // ON LOAD


    $rootScope.getFormData($scope, 'source', function (data) {
        if (!data._class) {
            data._class = 'none';
        }
        $rootScope.config.source = data;
    });



})


/**************************/
/******** DESTINATION *****/
/**************************/

.controller('DestinationController', function DestinationController($rootScope, $scope, $http, $location) {

    $scope.saveAndNext = function () {
        $scope.connect(function () {
            $location.path('/options');
        });
    };

    $scope.connect = function (callback) {
        $rootScope.config.destination._class = 'influx';
        $rootScope.connectRecord($rootScope.config.destination, 'destination', function () {
            console.log('success');
            typeof callback == "function" && callback();
        });
    };


    // ON LOAD

    $rootScope.getFormData($scope,'destination',function (data) {
        $rootScope.config.destination = data;
    });
})

/**************************/
/******** OPTIONS *********/
/**************************/
.controller('OptionsController', function OptionsController($rootScope, $scope, $http, $location) {

    var wrapper = {};
    var rowTemplate = {};


    $scope.saveAndNext = function () {
        // send options, then tell root scope to get/save the whole config.
        $http({
            method: 'POST',
            url: 'http://localhost:3131/options',
            headers: { 'Content-Type': undefined }, // undefined content-type to overcome CORS
            data: $scope.config.fetch
        }).then(function (response) {
            $rootScope.saveConfig();
            $location.path('/run');
        }, function (response) {
            console.log(response);
            $rootScope.notifyHttpError(response);
        });
    }; // saveAndNext


    $scope.sendGrafana = function () {

        $rootScope.showInfo("Sending Dashboards");

        var dash = JSON.parse(JSON.stringify(wrapper)); // json stringify-parse to deep copy
        dash.dashboard.title = 'LINK Historian';
        angular.forEach($rootScope.config.series, function (series) {
            var newRow = JSON.parse(JSON.stringify(rowTemplate));

            newRow.panels[0].leftYAxisLabel = series.units.unitString;
            newRow.panels[0].targets[0].query = 'select mean(value) from ' + series.name;
            newRow.panels[0].targets[0].alias = '$m';
            newRow.panels[0].title = series.name;
            newRow.title = series.name;

            dash.dashboard.rows.push(newRow);
        });

        // send dashboards

        // relay data to Node, since we can't make real POST requests otherwise.
        $http({
            method: 'POST',
            url: 'http://localhost:8585/send_dashboards',
            headers: {
                'Content-Type': 'application/json'
            },
            data: {login: $rootScope.config.dash, data: dash}
        }).then(function (response) {
            console.log('SENT DASHBOARDS');
            $rootScope.showInfo(response.data.message);
        }, function (response) {
            $rootScope.showInfo('');
            $rootScope.showError(response.data.message);
            console.log(response);
        });

    };

    $scope.setProto = function (p) {
        $rootScope.config.dash.proto = p;
    }


    // ON LOAD

    $rootScope.getFormData($scope,'options', function (data) {
        $rootScope.config.fetch = data;
    });


    //-- get grafana wrapper templates for later --//
    $http.get('/grafana/grafana_wrapper.json')
        .then(function (response) {
            wrapper = response.data;
        }, function (err) {
            console.log('err getting wrapper');
        });
    $http.get('/grafana/grafana_contents.json')
        .then(function (response) {
            rowTemplate = response.data;
        }, function (err) {
            console.log('err getting row template');
        });
})


/**************************/
/******** RUN     *********/
/**************************/
.controller('RunController', function RunController($rootScope, $scope, $http, $location, $interval) {
    $scope.status = 'Checking...';
    $scope.run = false;

    $scope.setRun = function (shouldRun) {
        $scope.run = shouldRun;
    };

    $scope.refreshStatus = function() {
        $scope.status = "Checking...";
        $http.get('http://localhost:3131/run')
            .then(function(response) {
                $scope.status = response.data.run ? 'Running' : 'Idle';
            }, function(response) {
                console.log('Error: ' + response);
                $scope.status = "Error communicating with RTX Service.";
            });
    };

    $scope.refreshStatus();
    var refreshInterval = $interval($scope.refreshStatus, 5000);
    $scope.$on('$destroy', function () { $interval.cancel(refreshInterval); });
})

.controller('AboutController', function AboutController($scope, $http) {
    $scope.author = 'OWA';
});
