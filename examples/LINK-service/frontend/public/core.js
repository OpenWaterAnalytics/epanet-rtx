// public/core.js
var rtxLink = angular.module('rtxLink', ['ngRoute'])

    .run(function ($rootScope, $timeout, $http, $interval) {

        $rootScope.config = {
            source:      {},
            destination: {},
            fetch:       { window:12, interval:5, backfill:30},
            series:      [],
            dash:        { proto:'http' }
        };

        $rootScope.sourceTypes = {
            'odbc': {
                subOptions: ["simple", "advanced"],
                inputRows: [
                    {
                        inputType:'text-line',
                        key:'name',
                        text:'Name',
                        placeholder:'SCADA User-Defined Name'
                    },{
                        inputType:'select-line',
                        key:'driver',
                        text:'ODBC Driver',
                        placeholder:'MS-SQL ODBC Provider'
                    },{
                        inputType:'text-line',
                        key:'simple_connection_ip',
                        text:'Historian IP Address',
                        placeholder:'192.168.0.1',
                        visible_when: 'simple'
                    },{
                        inputType:'text-line',
                        key:'connectionString',
                        text:'Connection',
                        placeholder:'SVR=192.168.0.1;blah=blah',
                        visible_when: 'advanced'
                    },{
                        inputType: 'text-line',
                        key: 'simple_meta_table',
                        text: 'Tag List Table',
                        placeholder: 'tag_list_table',
                        visible_when: 'simple'
                    },{
                        inputType: 'text-line',
                        key: 'simple_meta_col_tag',
                        text: '--------- Tag Column',
                        placeholder: 'tag_name_col',
                        visible_when: 'simple'
                    },{
                        inputType: 'text-line',
                        key: 'simple_meta_col_units',
                        text: '--------- Units Column',
                        placeholder: 'units_name_col',
                        visible_when: 'simple'
                    },{
                        inputType: 'text-line',
                        key: 'simple_range_table',
                        text: 'History Table',
                        placeholder: 'table_name',
                        visible_when: 'simple'
                    },{
                        inputType: 'text-line',
                        key: 'simple_range_col_date',
                        text: '--------- Date Column',
                        placeholder: 'date_col',
                        visible_when: 'simple'
                    },{
                        inputType: 'text-line',
                        key: 'simple_range_col_value',
                        text: '--------- Value Column',
                        placeholder: 'value_col',
                        visible_when: 'simple'
                    },{
                        inputType: 'text-line',
                        key: 'simple_range_col_quality',
                        text: '--------- Quality Column',
                        placeholder: 'quality_col',
                        visible_when: 'simple'
                    },{
                        inputType: 'text-line',
                        key: 'simple_range_col_tag',
                        text: '--------- Tag Column',
                        placeholder: 'tag_col',
                        visible_when: 'simple'
                    },{
                        inputType: 'text-line',
                        key:'meta',
                        text: 'Name lookup query',
                        placeholder: 'SELECT tagname, units FROM tag_list ORDER BY tagname ASC',
                        visible_when: 'advanced'
                    },{
                        inputType: 'text-area',
                        key: 'range',
                        text: 'Data lookup query',
                        placeholder: 'SELECT date, value, quality FROM tbl WHERE tagname = ? AND date >= ? AND date <= ? ORDER BY date ASC',
                        visible_when: 'advanced'
                    }
                ]
            },
            'sqlite': {
                inputRows: [
                    {
                        key: 'name',
                        text: 'Name',
                        placeholder: 'User-defined Name',
                        inputType: 'text-line'
                    },
                    {
                        key: 'connectionString',
                        text: 'File Name',
                        placeholder: 'tempfile.sqlite',
                        inputType: 'text-line'
                    }
                ]
            },
            'influx': {
                inputRows: [
                    {
                        key: 'name',
                        text: 'Name',
                        placeholder: 'User-defined Name',
                        inputType: 'text-line'
                    },
                    {
                        key: 'connectionString',
                        text: 'Connection',
                        placeholder: 'host=http://flux.citilogics.io&u=user&p=pass',
                        inputType: 'text-line'
                    }
                ]
            }
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

        $rootScope.notifyHttpError = function (response) {
            if (response.data) {
                console.log("got back form server:");
                console.log(response.data);
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
                errStr = 'Error: NODE service offline or unreachable.';
            }
            $rootScope.showError(errStr);
        };

        $rootScope.saveConfig = function () {
            // get config from LINK Server, send to Node Server.
            // Node will save the config locally.
            $rootScope.relayGet('config',
                function (response) {
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

                    $rootScope.relayPost('config', data,
                        function (response) {
                            // success
                            console.log('Successfully sent configuration to RTX-LINK.');
                            console.log(response);
                        }, function (response) {
                            // error
                            $rootScope.notifyHttpError(response);
                        });

                }, function (errResponse) {
                    // error
                    $rootScope.notifyHttpError(errResponse);
                });
        };

        $rootScope.getFormData = function (urlPath,callback) {
            $rootScope.relayGet(urlPath,function (successResponse) {
                typeof callback == "function" && callback(successResponse.data);
            }, function (errResponse) {
                $rootScope.notifyHttpError(errResponse);
            });
        };


        $rootScope.ping = function() {
            $rootScope.relayGet('ping',
                function (response) {
                    // this is fine
                }, function (response) {
                    $rootScope.notifyHttpError(response);
                });
        };

        $rootScope.relayGet = function (path, successCallback, failureCallback) {
            $http.get('http://localhost:8585/link-relay/' + path)
                .then(function (response) {
                    typeof successCallback == "function" && successCallback(response);
                }, function (errResponse) {
                    typeof failureCallback == "function" && failureCallback(errResponse);
                });
        };

        $rootScope.relayPost = function (path, data, successCallback, failureCallback) {
            console.log("sending to node: ");
            console.log(data);
            $http({
                method: 'POST',
                url: 'http://localhost:8585/link-relay/' + path,
                headers: {'Content-type': 'application/json'},
                data: data
            }).then(function (response) {
                typeof successCallback == "function" && successCallback(response);
            }, function (errResponse) {
                typeof failureCallback == "function" && failureCallback(errResponse);
            });
        };


        // ON LOAD
        var pingInterval = $interval($rootScope.ping, 1000);
        $rootScope.$on('$destroy', function () { $interval.cancel(pingInterval); });
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
            .when('/dashboard', {
                templateUrl: 'templates/dash.part.html',
                controller: 'DashController'
            })
            .otherwise({
                redirectTo: '/main'
            });
    }])

    .controller('OdbcDriverSelectController', function OdbcDriverSelectController($rootScope, $scope, $http) {
        $scope.drivers = ["getting driver list..."];
        $rootScope.relayGet('odbc', function (response) {
            $scope.drivers = response.data;
            if ($scope.drivers.length == 0) {
                $scope.drivers = ["No Drivers Installed. Please install an ODBC Driver."];
                $scope.config.source.driver = $scope.drivers[0];
            }
        }, function (response) {
            $scope.drivers = ["failed to get driver list"];
        });

    })

    .controller('MainController', function MainController($scope, $http, $interval) {
        // nothing
    })


    /**************************/
    /******** SOURCE **********/
    /**************************/

    .controller('SourceController', function SourceController($rootScope, $scope, $http, $interval, $location) {

        $scope.saveAndNext = function () {
            var tsList = $scope.sourceSeries.filter(function (v) { return v._link_selected; });
            $rootScope.config.series = tsList;

            $rootScope.relayPost('series', tsList,
                function (response) {
                    $location.path('/destination');
                }, function (response) {
                    $rootScope.notifyHttpError(response);
                });
        };

        // get units list
        var getUnits = function () {
            $rootScope.relayGet('units',
                function (response) {
                    $scope.availableUnits = response.data;
                }, function (response) {
                    $scope.availableUnits = ["error: server offline","check connection"];
                });
        };

        $scope.isType = function (typeName) {
            return typeName === $rootScope.config.source._class;
        };
        $scope.setSourceType = function (typeName) {
            $rootScope.config.source = {_class: typeName};
            $scope.sourceSeries = [];
            $scope.showTsList = false;
            if ($rootScope.sourceTypes[typeName].subOptions) {
                $scope.subOption = $rootScope.sourceTypes[typeName].subOptions[0];
            }
            else {
                $scope.subOption = "";
            }

        };

        $scope.connect = function () {
            $rootScope.relayPost('source', $scope.config.source,
                function (response) {
                    $rootScope.showInfo("Connection OK");
                    $scope.refreshSeriesList();
            }, function (errResponse) {
                    $rootScope.notifyHttpError(errResponse);
            });
        };

        $scope.refreshSeriesList = function () {
            $rootScope.showInfo('Connected. Fetching List of Series...')
            getUnits();
            $rootScope.relayGet('source/series',
                function (response) {
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

        $scope.removeSeries = function (selected) {
            $scope.sourceSeries = $scope.sourceSeries.filter(function (v) {
                return v._link_selected != selected;
            });
        };

        $scope.reconcileSeries = function () {
            // get the list of dupe series from LINK
            // and match them to the list of series (if any)
            // in the searchable list, and make sure they are selected.
            $rootScope.relayGet('series',
                function (response) {
                    var dupeSeries = response.data;
                    var dupeNames = [];
                    angular.forEach(dupeSeries, function (series) {
                        dupeNames.push(series['name']);
                    });
                    angular.forEach($scope.sourceSeries, function (series) {
                        series._link_selected = (-1 !== dupeNames.indexOf(series.name));
                    });
                }, function (response) {

                });
        };

        $scope.isSubOption = function (option) {
            return $scope.subOption == option;
        };
        $scope.setSubOption = function (option) {
            $scope.subOption = option;
        };

        $scope.refreshAdvanced = function (visible_when) {
            if (visible_when && visible_when.indexOf('simple') > -1) {
                if ($scope.config.source._class == "odbc") {
                    var r = $scope.config.source;
                    $scope.config.source.meta = "SELECT " + r.simple_meta_col_tag + ", " + r.simple_meta_col_units + " FROM " + r.simple_meta_table;
                    $scope.config.source.connectionString = "SRV=" + r.simple_connection_ip;
                    $scope.config.source.range =
                        "SELECT " + r.simple_range_col_date +
                        ", " + r.simple_range_col_value +
                        ", " + r.simple_range_col_quality +
                        " FROM " + r.simple_range_table +
                        " WHERE " + r.simple_range_col_tag + " = ?" +
                        " AND " + r.simple_range_col_date + " >= ? " +
                        " AND " + r.simple_range_col_date + " <= ? " +
                        " ORDER BY " + r.simple_range_col_date + " ASC";
                }
            }
        };

        // ON LOAD
        $scope.subOption = "";
        $scope.availableUnits = [];
        $rootScope.config.source = {_class: 'none'};
        $scope.sourceSeries = [];
        $scope.statusMsg = '';
        $scope.showTsList = false;

        $rootScope.getFormData('source', function (data) {
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
            $rootScope.relayPost('destination', $rootScope.config.destination,
                function (response) {
                    typeof callback == "function" && callback();
                }, function (errResponse) {
                    $rootScope.notifyHttpError(errResponse);
                });
        };

        // ON LOAD
        $rootScope.getFormData('destination',function (data) {
            $rootScope.config.destination = data;
        });
    })

    /**************************/
    /******** OPTIONS *********/
    /**************************/
    .controller('OptionsController', function OptionsController($rootScope, $scope, $http, $location) {

        $scope.saveAndNext = function () {
            // send options, then tell root scope to get/save the whole config.
            $rootScope.relayPost('options', $scope.config.fetch,
                function (response) {
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
                newRow.panels[0].targets[0].query = 'select mean(value) from "' + series.name + '" WHERE $timeFilter GROUP BY time($interval) fill(null)';
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
        };


        // ON LOAD
        var wrapper = {};
        var rowTemplate = {};

        $rootScope.getFormData('options', function (data) {
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

        $scope.setRun = function (shouldRun) {
            $rootScope.relayPost('run', {run: shouldRun},
                function (response) {
                    // sent ok
                }, function (errResponse) {
                    $rootScope.showInfo('');
                    $rootScope.notifyHttpError(errResponse);
                });
        };

        $scope.refreshStatus = function() {
            $scope.status = "Checking...";
            $rootScope.getFormData('run',function(data) {
                $scope.runInfo = data;
                $scope.status = data.run ? 'Running: ' + data.message : 'Idle';
            });
        };

        // ON LOAD
        $scope.status = 'Checking...';
        $scope.runInfo = {
            run: false,
            progress: 0,
            message: ''
        };
        $scope.runCmdText = 'Run';
        $scope.refreshStatus();
        var refreshInterval = $interval($scope.refreshStatus, 1000);
        $scope.$on('$destroy', function () { $interval.cancel(refreshInterval); });
    })

    .controller('DashController', function DashController($rootScope, $scope, $http, $location, $interval, $sce) {
        $scope.frameSrc = $rootScope.config.dash.proto + '://' + $rootScope.config.dash.url + '/dashboard/db/link-historian';
        $scope.trustSrc = function(src) {
            return $sce.trustAsResourceUrl(src);
        };
    })

    .controller('AboutController', function AboutController($scope, $http) {
        $scope.author = 'OWA';
    });
