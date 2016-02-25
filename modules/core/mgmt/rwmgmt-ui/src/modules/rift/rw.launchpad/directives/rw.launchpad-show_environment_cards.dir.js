angular.module('launchPad')
    // Primary directive for displaying environment cards
    // Uses environmentCardsFactory to update the DOM
    .directive('showEnvironmentCards', ['$rootScope', 'showEnvironmentCardsFactory', 'dispatcher', function($rootScope, showEnvironmentCardsFactory, dispatcher) {
        "use strict";

        return {
            restrict: 'AE',
            template: '',
            scope: {
                data: '='
            },
            controller: function($scope, showEnvironmentCardsFactory, $timeout) {
                $scope.data = [];

                // first time
                showEnvironmentCardsFactory.getEnvironments()
                .then(function(payload) {
                    $scope.data = payload.data.environments;
                    dispatcher.emit('data-updated', $scope.data);
                });

                // further
                $scope.ws = showEnvironmentCardsFactory.openEnvironmentsSocket(function(data) {

                    // Map data.key with appropriate $scope.data entry
                    for (var ii = 0; ii < $scope.data.length; ii++) {
                        var lookedupObj = data[$scope.data[ii]['id']];
                        for (var key in lookedupObj) {
                            $scope.data[ii][key] = lookedupObj[key];
                        }
                    }
                    dispatcher.emit('data-updated', $scope.data);
                });

                dispatcher.watch('close-socket', function() {
                    showEnvironmentCardsFactory.closeEnvironmentsSocket($scope.ws);
                });

                // todo: where to do this? in on destroy?
                // setTimeout(function() {
                //     dispatcher.emit('close-socket');
                // }, 5000);
            },
            link: function(scope, element, attributes) {

                scope.$on('$destroy', function() {
                    dispatcher.emit('close-socket');                    
                });

                dispatcher.watch('data-updated', function(newData) {
                    React.render(
                        EnvironmentCard({
                            data: newData,
                            dispatcher: dispatcher
                        }),
                        document.getElementById('cards')
                    );
                });

                React.render(
                    EnvironmentCard({
                        data: scope.data,
                        dispatcher: dispatcher
                    }),
                    document.getElementById('cards')
                );
            }
        }
    }]);

