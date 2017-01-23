// public/core.js
var rtxLink = angular.module('rtxLink', ['ngRoute','ui.bootstrap'])

.run(function ($rootScope, $timeout, $http, $interval, $location) {
// the data model stub
  $rootScope.config = {
    source:      {},
    destination: {},
    options:       { window:12, interval:5, backfill:30},
    series:      [],
    dash:        { proto:'http' },
    run:         { run: false },
    analytics: [
      // {
    	//   type: "tank",
    	//   "geometry": {
      //     "tankId": "tank1_name",
    	//     "inputUnits": "ft",
    	//     "outputUnits": "ft3",
      //     "inletDiameter": 12,
      //     "inletUnits": "in",
    	//     "data": [
    	//       [1,1],
    	//       [2,2],
    	//       [3,3]
    	//     ]
    	//   }
    	// }
    ]
  };

  $rootScope.sourceTypes = {
    'odbc': {
      subOptions: ["simple", "advanced"],
      inputRows: [
        {
          inputType:'text-line',
          key:'name',
          text:'Name',
          placeholder:'SCADA User-Defined Name',
          helptext:'User-Defined Name. Type anything here.'
        },{
          inputType:'select-line',
          key:'driver',
          text:'ODBC Driver',
          placeholder:'Select ODBC Driver',
          helptext:'If you do not see your ODBC Driver, check that it is installed'
        },{
          inputType:'select-line-options',
          key:'zone',
          text:'Database Timezone',
          placeholder:'local',
          options:['local','utc'],
          helptext:'The Timezone of the date/time stamps in the database'
        },{
          inputType:'text-line',
          key:'simple_connection_ip',
          text:'Historian IP Address',
          placeholder:'192.168.0.1',
          visible_when: 'simple'
        },{
          inputType:'text-line',
          key:'simple_connection_db_name',
          text:'Database Name',
          placeholder:'Historian_DB',
          visible_when: 'simple'
        },{
          inputType:'text-line',
          key:'simple_connection_user',
          text:'User Name',
          placeholder:'rtx_readonly_user',
          visible_when: 'simple'
        },{
          inputType:'text-line',
          key:'simple_connection_password',
          text:'Password',
          placeholder:'rtx_readonly_password',
          visible_when: 'simple'
        },{
          inputType:'text-line',
          key:'simple_connection_extra',
          text:'Extra Parameters',
          placeholder:'TDS_Version=7.1;Port=1433',
          visible_when: 'simple'
        },{
          inputType:'text-line',
          key:'connectionString',
          text:'Connection',
          placeholder:'Server=192.168.0.1;Port=1433;TDS_Version=7.1;Database=Historian_DB;UID=rtx;PWD=rtx',
          visible_when: 'advanced',
          helptext: 'ODBC Connection String. Enter the host, port, database, uid, pwd, and any driver-specific options'
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
          visible_when: 'advanced',
          helptext: 'A query that will return the list of tag names, and optionally a column of unit strings.'
        },{
          inputType: 'text-area',
          key: 'range',
          text: 'Data lookup query',
          placeholder: 'SELECT date, value, quality FROM tbl WHERE tagname = ? AND date >= ? AND date <= ? ORDER BY date ASC',
          visible_when: 'advanced',
          helptext: 'A query that will return (date,value,quality) columns with three ? placeholders specifying the tagname, start date, and end date.'
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
    'opc': {
      inputRows: [
        {
          key: 'name',
          text: 'Name',
          placeholder: 'User-defined Name',
          inputType: 'text-line'
        },
        {
          key: 'connectionString',
          text: 'Endpoint',
          placeholder: 'opc.tcp://192.168.0.100/Devices/',
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

  $rootScope.destinationTypes = {
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
          placeholder: 'host=http://flux.citilogics.io&port=8086&db=dest_db&u=user&p=pass',
          inputType: 'text-line'
        }
      ]
    },
    'influx_udp': {
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
          placeholder: 'host=http://flux.citilogics.io&port=8089',
          inputType: 'text-line'
        }
      ]
    }
  };

  $rootScope.rtxTypes = {
    'odbc': 'ODBC',
    'sqlite': 'SQLite',
    'influx': 'Influx',
    'influx_udp': "Influx-UDP",
    'opc': 'OPC'
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
      console.log("got back from server:");
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
      console.log(response);
    }
    $rootScope.showError(errStr);
  };

  $rootScope.ping = function() {
    $rootScope.relayGet('ping',
    function (response) {
      // this is fine
    }, function (response) {
      console.log('error in ping');
      $rootScope.notifyHttpError(response);
    });
  };

  $rootScope.relayGet = function (path, successCallback, failureCallback) {
    $http.get('http://' + $location.host() + ':8585/relay/' + path)
    .then(function (response) {
      typeof successCallback == "function" && successCallback(response);
    }, function (errResponse) {
      typeof failureCallback == "function" && failureCallback(errResponse);
    });
  };

  // send data from configuration object to NODE server.
  $rootScope.postConfig = function (path, successCallback, failureCallback) {
    var data = $rootScope.config[path];
    console.log("sending to node: ");
    console.log(data);
    $http({
      method: 'POST',
      url: 'http://' + $location.host() + ':8585/config/' + path,
      headers: {'Content-type': 'application/json'},
      data: data
    }).then(function (response) {
      typeof successCallback == "function" && successCallback(response);
    }, function (errResponse) {
      typeof failureCallback == "function" && failureCallback(errResponse);
    });
  };

  /////////////////////////////////
  // ON LOAD    ///////////////////
  /////////////////////////////////
  var pingInterval = $interval($rootScope.ping, 1000);
  $rootScope.$on('$destroy', function () { $interval.cancel(pingInterval); });

  // get the complete configuration object from NODE
  $http.get('http://' + $location.host() + ':8585/config')
  .then(function (response) {
    $rootScope.config = response.data;
    if (!$rootScope.config.dash) {
      $rootScope.config.dash = {proto: 'http'};
    }
  }, function (errResponse) {
    $rootScope.notifyHttpError(errResponse);
  });


  $rootScope.wrapper = {};
  $rootScope.rowTemplate = {};

  // prepare with grafana wrappers
  //-- get grafana wrapper templates for later --//
  $http.get('/grafana/grafana_wrapper.json')
  .then(function (response) {
    $rootScope.wrapper = response.data;
  }, function (err) {
    console.log('err getting wrapper');
  });
  $http.get('/grafana/grafana_contents.json')
  .then(function (response) {
    $rootScope.rowTemplate = response.data;
  }, function (err) {
    console.log('err getting row template');
  });


})

.controller('HeaderController', function HeaderController($scope, $location) {
  $scope.links = [
    //{url:'main', text:'Main'},
    {url:'source', text:'Source'},
    {url:'destination', text:'Destination'},
    {url:'analytics', text:'Analytics'},
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
  .when('/analytics', {
    templateUrl: 'templates/analytics.part.html',
    controller: 'AnalyticsController'
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
    $rootScope.postConfig('series',
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
      $rootScope.config.source.inputOption = $rootScope.sourceTypes[typeName].subOptions[0];
    }
    else {
      $rootScope.config.source.inputOption = "";
    }
  };

  $scope.connect = function () {
    $rootScope.postConfig('source',
      function (response) {
        $rootScope.showInfo("Sent Configuration");
        $scope.refreshSeriesList();
      }, function (errResponse) {
        $rootScope.notifyHttpError(errResponse);
      });
  };

  $scope.refreshSeriesList = function () {
    $rootScope.showInfo('Connecting and Fetching List of Series...')
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

  $scope.deselectAll = function () {
    angular.forEach($scope.sourceSeries, function(item) {
      item._link_selected = false
    });
  };

  $scope.reconcileSeries = function () {
    // and match dupe series to the list of series (if any)
    // in the searchable list, and make sure they are selected.
    var dupeSeries = $rootScope.config.series;
    var dupeNames = [];
    angular.forEach(dupeSeries, function (series) {
      dupeNames.push(series['name']);
    });
    angular.forEach($scope.sourceSeries, function (series) {
      var dupeIndex = dupeNames.indexOf(series.name);
      series._link_selected = (-1 !== dupeIndex);
      if (-1 !== dupeIndex) {
        series.units = dupeSeries[dupeIndex].units
      }
    });

  };

  $scope.isSubOption = function (option) {
    return $rootScope.config.source.inputOption == option;
  };
  $scope.setSubOption = function (option) {
    $rootScope.config.source.inputOption = option;
  };

  $scope.refreshAdvanced = function (visible_when) {
    if (visible_when && visible_when.indexOf('simple') > -1) {
      if ($scope.config.source._class == "odbc") {
        var r = $scope.config.source;
        // Server=10.0.1.20;Port=1433;TDS_Version=7.1;Database=SCADA;UID=rtx;PWD=sqladmin
        $scope.config.source.meta = "SELECT " + r.simple_meta_col_tag + ", " + r.simple_meta_col_units + " FROM " + r.simple_meta_table;
        $scope.config.source.connectionString =
        "Server=" + r.simple_connection_ip +
        ";Database=" + r.simple_connection_db_name +
        ";UID=" + r.simple_connection_user +
        ";PWD=" + r.simple_connection_password +
        ";" + r.simple_connection_extra;
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
  $scope.availableUnits = [];
  //$rootScope.config.source = {_class: 'none'};
  $scope.sourceSeries = [];
  $scope.statusMsg = '';
  $scope.showTsList = false;
  getUnits();

})


/**************************/
/******** DESTINATION *****/
/**************************/

.controller('DestinationController', function DestinationController($rootScope, $scope, $http, $location) {

  $scope.saveAndNext = function () {
    $scope.connect(function () {
      $rootScope.showInfo('Connection Successful');
      $location.path('/options');
    }, function(err) {
      $rootScope.notifyHttpError(err);
    });
  };

  $scope.connect = function (callback, errCallback) {
    $scope.connectionMessage = "Trying Connection...";
    $scope.connectionStatus = "wait";
    $rootScope.postConfig('destination',
      function (response) {
        $scope.connectionMessage = "Connection Successful";
        $scope.connectionStatus = "ok";
        typeof callback == "function" && callback();
      }, function (errResponse) {
        errStr = "Unknown Error";
        if (errResponse.data.error) {
          errStr = errResponse.data.error;
        }
        else if (errResponse.data.message) {
          errStr = errResponse.data.message;
        }
        $scope.connectionMessage = "ERROR: " + errStr;
        $scope.connectionStatus = "err";
        typeof errCallback == "function" && errCallback();
    });
  };

  $scope.isType = function (typeName) {
    return typeName === $rootScope.config.destination._class;
  };
  $scope.setDestinationType = function (typeName) {
    $rootScope.config.destination = {_class: typeName};
  };




  // ON LOAD
  $scope.connectionMessage = "";
  $scope.connectionStatus = "none";
})




/**************************/
/******* ANALYTICS ********/
/**************************/
.controller('AnalyticsController', function AnalyticsController($rootScope, $scope, $http, $location) {
  $scope.saveAndNext = function () {
    $rootScope.postConfig('analytics',
      function (response) {
        $location.path('/options');
      }, function (response) {
        $rootScope.notifyHttpError(response);
      });
  };

  $scope.addAnalytic = function() {
    if (!$rootScope.config.analytics) {
      $rootScope.config.analytics = [];
    }
    $rootScope.config.analytics.push({"type":"New Analytic","_isOpen":true});
    console.log("added analytic");
  }
  $scope.removeAnalytic = function(a) {
    console.log('removing: ' + a);
    var index = $rootScope.config.analytics.indexOf(a);
    if (index > -1) {
      $rootScope.config.analytics.splice(index, 1);
    }
  }
  $scope.addGeometryPoint = function(analytic) {
    analytic.geometry.data.push([0.0,0.0]);
  }

  // watch the analytics array and do validation or other custom actions
  $rootScope.$watch('config.analytics',function(val,old){
      angular.forEach($rootScope.config.analytics, function (a) {
        a.name = a.type;
        // if tank type
        if (a.type === 'tank') {
          if (!("geometry" in a)) {
            a.geometry = {};
            a.geometry.data = [];
          }
          if ("geometry" in a && "tankId" in a.geometry) {
            var geo = a.geometry;
            a.name += ' ' + geo.tankId;
          }

          if ("_tankTs" in a) {
            a.geometry.tankId = a._tankTs.name;
          }
        } // tank
      }); // analytic
    }, true /* true = watch deep */);

  // validation for point field types -> number
  $scope.validatePoint = function(pt) {
    angular.forEach([0,1], function(idx) {
      if (typeof pt[idx] === 'string') {
        pt[idx] = parseFloat(pt[idx]);
      }
    });// field
  };

  $scope.logState = function() {
    console.log($rootScope.config.analytics);
  }

  // on load
  $scope.candidateSeries = $rootScope.config.series;
  $scope.availableAnalytics = ["tank","none"];
})



/**************************/
/******** OPTIONS *********/
/**************************/
.controller('OptionsController', function OptionsController($rootScope, $scope, $http, $location) {

  $scope.saveAndNext = function () {
    // send dash credentials
    $rootScope.postConfig('dash',
  function(response) {
    // send options, then tell root scope to get/save the whole config.
    $rootScope.postConfig('options',
    function (response) {
      $location.path('/run');
    }, function (errorResponse) {
      console.log(errorResponse);
      $rootScope.notifyHttpError(errorResponse);
    });
  }, function(errorResponse) {
      console.log(errorResponse);
      $rootScope.notifyHttpError(errorResponse);
  });

  }; // saveAndNext

  $scope.sendGrafana = function () {
    $rootScope.showInfo("Sending Dashboards");
    var dash = JSON.parse(JSON.stringify($rootScope.wrapper)); // json stringify-parse to deep copy
    dash.dashboard.title = 'LINK Historian';
    var panelId = 1;
    angular.forEach($rootScope.config.series, function (series) {
      var newRow = JSON.parse(JSON.stringify($rootScope.rowTemplate));
      newRow.panels[0].id = panelId;
      newRow.panels[0].yaxes[0].label = series.units.unitString;
      newRow.panels[0].targets[0].query = 'select mean("value") from "' + series.name + '" WHERE $timeFilter GROUP BY time($interval) fill(null)';
      newRow.panels[0].targets[0].alias = '$m';
      newRow.panels[0].title = series.name;
      newRow.title = series.name;
      dash.dashboard.rows.push(newRow);
      panelId += 1;
      console.log(newRow);
    });

    // send dashboards
    // relay data to Node, since we can't make real POST requests otherwise.
    $http({
      method: 'POST',
      url: 'http://' + $location.host() + ':8585/send_dashboards',
      headers: {
        'Content-Type': 'application/json'
      },
      data: {login: $rootScope.config.dash, data: dash}
    }).then(function (response) {
      console.log('SENT DASHBOARDS');
      $scope.dashStatus = "ok";
      $scope.dashCreateMessage = response.data.message;
    }, function (errResponse) {
      errStr = "Unknown Error";
      if (errResponse.data.error) {
        errStr = errResponse.data.error;
      }
      else if (errResponse.data.message) {
        errStr = errResponse.data.message;
      }
      $scope.dashCreateMessage = "INFO: " + errStr;
      $scope.dashStatus = "err";
      console.log(errResponse);
    });

  };

  $scope.setProto = function (p) {
    $rootScope.config.dash.proto = p;
  };


  // ON LOAD
  $scope.dashStatus = "none";
  $scope.dashCreateMessage = "";

})


/**************************/
/******** RUN     *********/
/**************************/
.controller('RunController', function RunController($rootScope, $scope, $http, $location, $interval) {

  $scope.setRun = function (shouldRun) {
    $rootScope.config.run = {run: shouldRun};
    $rootScope.postConfig('run',
      function (response) {
        // sent ok
      }, function (errResponse) {
        $rootScope.showInfo('');
        $rootScope.notifyHttpError(errResponse);
    });
  };

  $scope.refreshStatus = function() {
    $scope.status = "Checking...";
    $rootScope.relayGet('run', function(response) {
      $scope.runInfo = response.data;
      $scope.status = $scope.runInfo.run ? 'Running: ' + $scope.runInfo.message : 'Idle';
      $rootScope.config.run = $scope.runInfo;
    }, function(err) {
      // nothing
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


/*******************/
/**** DASHBOARD ****/
/*******************/

.controller('DashController', function DashController($rootScope, $scope, $http, $location, $interval, $sce, $timeout) {
  $scope.frameSrc = $rootScope.config.dash.proto + '://' + $rootScope.config.dash.url + '/dashboard/db/link-historian';
  $scope.trustSrc = function(src) {
    return $sce.trustAsResourceUrl(src);
  };
})

.controller('AboutController', function AboutController($scope, $http) {
  $scope.author = 'OWA';
});
