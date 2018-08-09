angular.module('bothApp', []).controller('bothController', [ '$location', '$scope', '$http', '$interval', function ($location, $scope, $http, $interval) {

    $scope.g_blobs = null;
    $scope.g_network = null;

    $scope.fetchedDataBlobs = [
        // empty to start with.
    ];

    $scope.fetchedDataNetwork = [
        // empty to start with.
    ];

    function RemoveLastDirectoryPartOf(the_url, removals)
    {
        var the_arr = the_url.split('/');
        var i;
        for (i = 0; i < removals; i++) {
            the_arr.pop();
        }
        return( the_arr.join('/') );
    }

    $scope.api_url_blobs = RemoveLastDirectoryPartOf($location.url(), 2) + "/consensus";
    $scope.api_url_network = RemoveLastDirectoryPartOf($location.url(), 2) + "/network";

    $scope.load = function() {
        $http.get($scope.api_url_blobs).then(
            function(response) {
                $scope.dataArrivedBlobs(response);
            });
        $http.get($scope.api_url_network).then(
            function(response) {
                $scope.dataArrivedNetwork(response);
            });
    }

    $scope.dataArrivedBlobs = function(response) {
        // process me here.
        $scope.fetchedDataBlobs = response.data;
        if (!$scope.g_blobs) {
            $scope.g_blobs = new BubbleForceGraph("blobs_here");
        }
        $scope.g_blobs.addData($scope.fetchedDataBlobs);
    }

    $scope.init = function() {
        $scope.load();
        $interval( $scope.load, 133);
    }

    $scope.dataArrivedNetwork = function(response) {
        $scope.fetchedDataNetwork = response.data;
        if (!$scope.g_network) {
            $scope.g_network = new ForceGraph("network_here", 250);
        }
        $scope.g_network.addData($scope.fetchedDataNetwork);
    }


    $scope.init();

}]).config(function ($locationProvider) {
    $locationProvider.html5Mode(true);
})
