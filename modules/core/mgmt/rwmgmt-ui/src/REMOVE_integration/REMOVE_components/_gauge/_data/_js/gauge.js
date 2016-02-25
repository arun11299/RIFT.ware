angular.module('webui.gauge', [])
    .directive('gauge', ['$interval',function($interval) {
        return {
            restrict: 'AE',
            scope: {
                value: '@initValue',
                size: '='
            },
            templateUrl: 'integration/_components/_gauge/gauge.html',
            link: function(scope, element, attrs) {
                scope.config = {
                    min: scope.min || 0,
                    max: scope.max || 100,
                    nSteps: scope.nsteps || 10,
                    size: scope.size || 300,
                    color: scope.color || 'hsla(212, 57%, 50%, 1)'
                };
//                angular.forEach(config, function(v, k) {
//                    if (!scope.config.hasOwnProperty(k)) {
//                        console.log('adding ' + k + ' with a value of ' + v)
//                        scope.config[k] = v;
//                    }
//                })
                console.log(scope.config)
                scope.$watch('scope.config.max', function() {
                    if (scope.config.max) {
                        renderGauge();
                    }
                });
                scope.$watch('value',function() {
                    scope.gauge.setValue(scope.value);
                })
                var renderGauge = function(callback) {
                        var range = scope.config.max - scope.config.min;
                        var step = Math.round(range / scope.config.nSteps);
                        var majorTicks = [];
                        for (var i = 0; i <= scope.config.nSteps; i++) {
                            majorTicks.push(scope.config.min + (i * step));
                        };
                        var redLine = scope.config.min + (range * 0.9);
                        var config = {
                            renderTo: element[0].lastElementChild,
                            width: scope.config.size,
                            height: scope.config.size,
                            glow: true,
                            units: false,
                            title: false,
                            minValue: scope.config.min,
                            maxValue: scope.config.max,
                            majorTicks: majorTicks,
                            minorTicks: 5,
                            strokeTicks: false,
                            highlights: [{
                                from: scope.config.min,
                                to: redLine,
                                color: scope.config.color
                            }, {
                                from: redLine,
                                to: scope.config.max,
                                color: 'hsla(8, 59%, 46%, 1)'
                            }],
                            colors: {
                                plate: '#3b3b3b',
                                majorTicks: '#ccc',
                                minorTicks: '#ccc',
                                title: '#ccc',
                                units: '#fff',
                                numbers: '#fff',
                                needle: {
                                    start: 'hsla(8, 59%, 46%, 1)',
                                    end: 'hsla(8, 59%, 46%, 1)'
                                }
                            }
                        };
                        if (scope.gauge) {
                            console.log('no gauge')
                            scope.gauge.updateConfig(config);
                            console.log(scope.gauge)
                        } else {
                            console.log('gauge')
                            scope.gauge = new Gauge(config);
                            scope.gauge.draw();
                            console.log(scope.gauge)
                        }
                        if(callback){
                            callback()
                        }
                    }
                renderGauge(function(){
                    $interval(function(){
                        scope.value = Math.floor(Math.random() * (scope.config.max - scope.config.min)) + scope.config.min;;
                    },1000)
                });


            },
            controller: function($scope,$interval) {
//                $interval(function(){
//                        console.log($scope.value)
//                       console.log('wtf')
//                        if($scope.value) {
//                           $scope.value = 99;
//                        }
//                    },200)
            }
        }
    }])