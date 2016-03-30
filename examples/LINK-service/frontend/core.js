// public/core.js
var rtxLink = angular.module('rtxLink', []);

rtxLink.controller("mainController", function mainController($scope, $http, $interval) {
    $scope.formData = {};
    $scope.series = [];
    $scope.status = "Unknown";

    // when landing on the page, get all series and show them
    $http.get('http://localhost:3131/series')
        .then(function(response) {
            $scope.series = response.data;
            console.log(response);
        }, function(response) {
            console.log('Error: ' + response);
        });

    $scope.refreshStatus = function() {
        $scope.status = "Checking...";
        $http.get('http://localhost:3131/run')
             .then(function(response) {
                 $scope.status = response.data.run ? "Running" : "Stopped";
              }, function(response) {
                 console.log('Error: ' + response);
              });
    };

    //$scope.refreshStatus();
    $interval($scope.refreshStatus, 5000);

}
);
