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

    $rootScope.connectRecord = function (formData, endpoint, callback) {
        $http({
            method: 'POST',
            url: 'http://localhost:3131/' + endpoint,
            headers: { 'Content-Type': undefined }, // undefined content-type to overcome CORS
            data: formData
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
            errStr = response.data['error'];
        } else {
            errStr = 'LINK Service Not Responding';
        }
        $rootScope.showError('Error Connecting: ' + errStr);
    }


})

.controller('HeaderController', function HeaderController($scope, $location) {
    $scope.links = [
        //{url:'main', text:'Main'},
        {url:'source', text:'Source'},
        {url:'destination', text:'Destination'},
        {url:'options', text:'Options'},
        {url:'run', text:'Run'},
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
    $scope.formData = {};
    $scope.series = [];

})


/**************************/
/******** SOURCE **********/
/**************************/

.controller('SourceController', function SourceController($rootScope, $scope, $http, $interval, $location) {
    $scope.availableUnits = [];
    $scope.formData = {_class: 'none'};
    $scope.sourceSeries = [];
    $scope.statusMsg = '';
    $scope.showTsList = false;

    $scope.getFormData = function () {
        $http.get('http://localhost:3131/source')
            .then(function (response) {
                $scope.formData = response.data;
                if (!$scope.formData._class) {
                    $scope.formData._class = 'none';
                }
            }, function (response) {
                $rootScope.notifyHttpError(response);
            });
    };

    $scope.saveAndNext = function () {
        var tsList = $scope.sourceSeries.filter(function (v) { return v._link_selected; });
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
        return typeName === $scope.formData._class;
    };
    $scope.setSourceType = function (typeName) {
        $scope.formData = {_class: typeName};
        $scope.sourceSeries = [];
        $scope.showTsList = false;
    };

    $scope.connect = function () {
        $rootScope.connectRecord($scope.formData, 'source', function () {
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
            }, function (response) {
                $rootScope.notifyHttpError(response);
            });
    };

    $scope.save = function () {
        console.log("form data: " + $scope.formData);
    };

    $scope.removeSeries = function (selected) {
        $scope.sourceSeries = $scope.sourceSeries.filter(function (v) {
            return v._link_selected != selected;
        });
    };

    // ON LOAD

    $scope.getFormData();



})


/**************************/
/******** DESTINATION *****/
/**************************/

.controller('DestinationController', function DestinationController($rootScope, $scope, $http, $location) {

    $scope.formData = {};

    $scope.getFormData = function () {
        $http.get('http://localhost:3131/destination')
            .then(function (response) {
                console.log(response);
                $scope.formData = response.data;
            }, function (response) {
                $rootScope.notifyHttpError(response);
            });
    };

    $scope.saveAndNext = function () {
        $scope.formData._class = 'influx';
        $rootScope.connectRecord($scope.formData, 'destination', function () {
            console.log('success');
            $location.path('/options');
        });
    };

    $scope.connect = function () {
        $scope.formData._class = 'influx';
        $rootScope.connectRecord($scope.formData, 'destination', function () {
            console.log('success');
        });
    };


    // ON LOAD

    $scope.getFormData();
})

/**************************/
/******** OPTIONS *********/
/**************************/
.controller('OptionsController', function OptionsController($rootScope, $scope, $http, $location) {

    $scope.formData = {
        interval: 15,
        window: 12,
        backfill: 30
    };

    $scope.saveAndNext = function () {
        // send POST to LINK service
        $http({
            method: 'POST',
            url: 'http://localhost:3131/options',
            headers: { 'Content-Type': undefined }, // undefined content-type to overcome CORS
            data: $scope.formData
        }).then(function (response) {
            $location.path('/run');
        }, function (response) {
            console.log(response);
            var errStr = response.data
                ? response.data['error']
                : 'LINK Service Not Responding';
            $rootScope.showError('Error Saving Options: ' + errStr);
        });
    }; // saveAndNext

})

.controller('RunController', function RunController($rootScope, $scope, $http, $location, $interval) {
    $scope.status = 'Checking...';

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
