angular.module('chainApp', []).controller('chainController', [ '$location', '$scope', '$http', '$interval', function ($location, $scope, $http, $interval) {

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
    $scope.api_url = RemoveLastDirectoryPartOf($location.url(), 2) + "/chain";

    $scope.init = function() {
        google.charts.load('current', {packages: ['orgchart']});
        google.charts.setOnLoadCallback($scope.ready);
    }

    $scope.ready = function() {
        $scope.load()
        $interval( $scope.load, 2000);
    }

    $scope.load = function() {
        $http.get($scope.api_url).then(
            function(response) {
                $scope.dataArrived(response);
            });
    }

    $scope.dataArrived = function(response) {
	// process me here.
        console.log("LOADED");
        $scope.fetchedData = response.data;
        $scope.createGraph(
            document.getElementById("chart_here"),
            $scope.dataToTable($scope.fetchedData)
        );
    }

    $scope.dataToTable = function(inp) {
        var data = new google.visualization.DataTable();
        data.addColumn('string', 'Name');
        data.addColumn('string', 'Manager');
        data.addColumn('string', 'ToolTip');

        inp.nodes.forEach(node => {
            data.addRows([
                [ node.name, node.manager, node.tooltip ],
            ]);
        });

        return data;
    }

    $scope.createGraph = function(el, datatable) {
        var foo = new google.visualization.OrgChart(el);
        var options = {
            title: "Blockchain",
            allowHtml:true,
        };
        foo.draw(datatable, options);
    }

    $scope.init();

}]).config(function ($locationProvider) {
    $locationProvider.html5Mode(true);
})
