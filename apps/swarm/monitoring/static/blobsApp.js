angular.module('blobsApp', []).controller('blobsController', [ '$location', '$scope', '$http', '$interval', function ($location, $scope, $http, $interval) {

    $scope.g = null;
    
    function RemoveLastDirectoryPartOf(the_url, removals)
    {
        var the_arr = the_url.split('/');
        var i;
        for (i = 0; i < removals; i++) {
            the_arr.pop();
        }
        return( the_arr.join('/') );
    }

    $scope.fetchedData = [
        // empty to start with.
    ];

    $scope.api_url = RemoveLastDirectoryPartOf($location.url(), 2) + "/consensus";

    $scope.load = function() {
        $http.get($scope.api_url).then(
            function(response) {
                $scope.dataArrived(response);
            });
    }

    $scope.dataArrived = function(response) {
	// process me here.
        $scope.fetchedData = response.data;
	if (!$scope.g) {
	    $scope.g = new BubbleForceGraph(null);
	}
	$scope.g.addData($scope.fetchedData);
    }

    $scope.init = function() {
        $scope.load();
	$interval( $scope.load, 300);
    }
    
    $scope.init();

}]).config(function ($locationProvider) {
    $locationProvider.html5Mode(true);
})
