// public/core.js
var rtxLink = angular.module('rtxLink', ['ngRoute']);

rtxLink.config(['$httpProvider', function($httpProvider) {
    $httpProvider.defaults.useXDomain = true;
    delete $httpProvider.defaults.headers.common['X-Requested-With'];
}
]);

rtxLink.config(['$routeProvider', function ($routeProvider) {
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
        .when('/series', {
            templateUrl: 'templates/series.part.html',
            controller: 'SeriesController'
        })
        .otherwise({
            redirectTo: '/main'
        });
}]);


rtxLink.controller('OdbcDriverSelectController', function OdbcDriverSelectController($scope, $http) {
    $scope.drivers = ["getting driver list..."];

    $http.get('http://localhost:3131/odbc')
        .then(function (response) {
            $scope.drivers = response.data;
        }, function (response) {
            $scope.drivers = ["failed to get driver list"];
        });

});

rtxLink.controller('MainController', function MainController($scope, $http, $interval) {
    $scope.formData = {};
    $scope.series = [];
    $scope.status = 'Checking...';


    var refreshStatus = function() {
        $scope.status = "Checking...";
        $http.get('http://localhost:3131/run')
            .then(function(response) {
                $scope.status = response.data.run ? 'Running' : 'Stopped';
            }, function(response) {
                console.log('Error: ' + response);
                $scope.status = "Error communicating with RTX Service.";
            });
    };

    refreshStatus();
    var refreshInterval = $interval(refreshStatus, 5000);
    $scope.$on('$destroy', function () { $interval.cancel(refreshInterval); });
});

rtxLink.controller('SourceController', function SourceController($scope, $http, $interval) {
    $scope.availableUnits = [];
    $scope.formData = {};
    $scope.sourceSeries = [];


    // get units list
    var getUnits = function () {
        $http.get('http://localhost:3131/units')
            .then(function (response) {
                $scope.availableUnits = response.data;
            }, function (response) {
                // failed
            });
    };



    $scope.sourceTypes = {
        'ODBC': [
            {
                key:'name',
                text:'Name',
                placeholder:'SCADA Colloquial Name',
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
        'SQLite': [
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
        'Influx': [
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
    $scope.selectedSourceType = 'ODBC';

    $scope.rtxTypes = {
        'ODBC': 'odbc',
        'SQLite': 'sqlite',
        'Influx': 'influx'
    };

    $scope.isType = function (typeName) {
        return typeName === $scope.selectedSourceType;
    };
    $scope.setSourceType = function (typeName) {
        $scope.formData = {};
        $scope.selectedSourceType = typeName;
        $scope.sourceSeries = [];
    };



    $scope.connect = function () {
        $scope.formData._class = $scope.rtxTypes[$scope.selectedSourceType];

        $http({
            method: 'POST',
            url: 'http://localhost:3131/source',
            headers: { 'Content-Type': undefined }, // undefined content-type to overcome CORS
            data: $scope.formData
        })
            .then(function (response) {
                console.log(response);
                $scope.refreshSeriesList()
            }, function (response) {
                console.log("error:: " + response);
            });

    };

    $scope.refreshSeriesList = function () {
        getUnits();
        $http.get('http://localhost:3131/source/series')
            .then(function (response) {
                $scope.sourceSeries = response.data;
                angular.forEach($scope.sourceSeries, function (series) {
                    series['_link_selected'] = false;
                });
                console.log($scope.sourceSeries);
            }, function (response) {
                // error
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
    
});

rtxLink.controller('SeriesController', function SeriesController($scope, $http) {
    $scope.series = [];
    $http.get('http://localhost:3131/series')
        .then(function(response) {
            $scope.series = response.data;
            console.log(response);
        }, function(response) {
            console.log('Error: ' + response);
        });
});

rtxLink.controller('AboutController', function AboutController($scope, $http) {
    $scope.author = 'OWA';
});

rtxLink.controller('HeaderController', function HeaderController($scope, $location) {
    $scope.links = [
        {url:'main', text:'Main'},
        {url:'source', text:'Source'},
        {url:'series', text:'Tags'},
        {url:'about', text:'About'}
    ];

    $scope.isActive = function (viewLocation) {
        return viewLocation === $location.path();
    };
});