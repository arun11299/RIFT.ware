module.exports = (function(window, angular) {
    "use strict";
    angular.module('cmdcntr').directive('resourcesDiagram', ['radio', resourcesDiagram]);


    function resourcesDiagram(radio) {


        function dottype(d) {
            d.x = +d.x;
            d.y = +d.y;
            return d;
        };

        function dragstarted(d) {
            d3.select(this).classed("dragging", true);
        };

        function dragged(d) {
            d3.select(this).attr("cx", d.x = d3.event.x).attr("cy", d.y = d3.event.y);
        };

        function dragended(d) {
            d3.select(this).classed("dragging", false);
        };

        function radius(area) {
            // area = (pi) r^2
            return Math.pow(area / Math.PI, 0.5);
        };

        function Controller(radio, $scope) {
            var self = this;
            var appChannel = radio.channel('appChannel');
            self.appChannel = appChannel;
            self.listeners = [];
            self.listeners.push(appChannel.on('diagram-update', function() {
                self.update();
            }, self));
            self.chart = 'linear';
            self.isDomReady = true;
            self.updateLegendUtilization();

            self.showType = {
                'linear'    : true,
                'radial'    : false
            };

            self.showMetric = {
                'cpu'       : true,
                'memory'    : false,
                'storage'   : false,
                'bps'       : false,
                'fabric'    : false
            };

            $(window).resize(function() {
                self.updateChartView();
            });

            $scope.$on('$stateChangeStart', function() {
                $(window).off('resize');

                self.resetSelection(self);


            });
            rw.BaseController.call(this);
        };
        Object.defineProperty(Controller.prototype, "sector", {
            enumerable: true,
            configurable: true,
            get: function() {
                return this._sector;
            },
            set: function(val) {
                this._sector = val;
                this.sectorChanged();
            }
        });
        angular.extend(Controller.prototype, {
            // How big do you draw a circle to represent 16 cores in a VM ?  Your answer
            // many vary from year to year as standard systems are produced with more cores. So
            // the magic numbers here are roughly based on the industry standard in the year 2014.
            utilization: {
                cpu: function(vm) {
                    return ('cpu' in vm ? vm.cpu.percent : 0);
                },
                storage: function(vm) {
                    return ('storage' in vm ? vm.storage.percent : 0);
                },
                bps: function(vm) {
                    return ('speed' in vm && vm.speed !== 0 ? 100 * ((vm.tx_rate_mbps + vm.rx_rate_mbps) / (2 * vm.speed)) : 0);
                },
                fabric: function(vm) {
                    var n = vm._fabricNode;
                    return ('speed' in n && n.speed !== 0 ? 100 * ((n.tx_rate_mbps + n.rx_rate_mbps) / (2 * n.speed)) : 0);
                },
                memory: function(vm) {
                    return ('memory' in vm ? vm.memory.percent : 0);
                }
            },
            capacity: {
                cpu: function(vm) {
                    return ('cpu' in vm ? radius(vm.cpu.ncores / 4) : 0.05);
                },
                storage: function(vm) {
                    return ('storage' in vm ? vm.storage.total / Math.pow(2, 25) : 0) ;
                },
                bps: function(vm) {
                    return (vm.speed ? radius(vm.speed / 5000) : 0.05);
                },
                fabric: function(vm) {
                    return (vm._fabricNode.speed ? radius(vm._fabricNode.speed / 5000) : 0.05);
                },
                memory: function(vm) {
                    return ('memory' in vm ? vm.memory.total / Math.pow(2, 25) : 0);
                }
            },
            colony: null,
            isDomReady: null,
            nodeRadius: 20,
            metric: 'cpu',
            legendCapacity: {
                cpu: '# VCPUs',
                memory: 'RAM Capacity',
                storage: 'Storage Capacity',
                bps: 'Gbps Capacity',
                fabric: 'Fabric Gbps Capacity'
            },
            fills: {
                cpu: ['hsla(120, 43%, 96%, 1)', 'hsla(125, 46%, 83%, 1)', 'hsla(123, 45%, 67%, 1)', 'hsla(123, 45%, 50%, 1)'],
                memory: ['hsla(31, 100%, 96%, 1)', 'hsla(32, 98%, 83%, 1)', 'hsla(32, 98%, 66%, 1)', 'hsla(27, 98%, 57%, 1)'],
                storage: ['hsla(177, 67%, 94%, 1)', 'hsla(177, 63%, 76%, 1)', 'hsla(180, 60%, 55%, 1)', 'hsla(180, 78%, 25%, 1)'],
                bps: ['hsla(212, 55%, 83%, 1)', 'hsla(211, 56%, 67%, 1)', 'hsla(212, 57%, 50%, 1)', 'hsla(211, 56%, 40%, 1)'],
                fabric: ['hsla(316, 100%, 94%, 1)', 'hsla(316, 100%, 88%, 1)', 'hsla(316, 100%, 79%, 1)', 'hsla(316, 100%, 70%, 1)']
            },
            sectorChanged: function() {
                if (this.sector !== undefined && this.sector && this.sector.collection && this.sector.collection.length > 0) {
                    this.colony = this.sector.collection[0];
                    this.updateChartView();
                    this.update();
                }
            },
            selectMetric: function(metric) {
                this.metric = metric;
                for (var key in this.showMetric) {
                    if (key == metric) {
                        this.showMetric[key] = true;
                    } else {
                        this.showMetric[key] = false;
                    }
                };
                this.updateLegendUtilization();
            },
            selectChart: function(type) {
                this.chart = type;
                for (var key in this.showType) {
                    if (key == type) {
                        this.showType[key] = true;
                    } else {
                        this.showType[key] = false;
                    }
                };
                this.updateChartView();
            },
            updateChartView: function() {
                d3.select('#diagram').selectAll(['text', 'g']).remove();
                this.newChart();
            },
            savedNode: {},
            theAsync: function() {
                var self = this;
                setTimeout(function() {
                    //TODO: How to do a scoped search?
                    // this.selectNode(this.colony, this.shadowRoot.getElementById('rootColony'));
                    self.selectNode(self.colony, $('#rootColony')[0]);
                }, 1000);
            },
            selectNode: function(node, svg) {
                var self = this;
                if (!this.isDomReady) {
                    return;
                }
                self.appChannel.trigger('node-selected', node, self);
                if (this.pointer) {
                    this.pointer.remove();
                }
                this.updateSelection(this);
                svg.style.strokeWidth = 4;
                svg.isActive = true;
                svg.attributes['stroke'].value = "hsla(198, 83%, 61%, 1.0)";
                this.savedNode = node;
                this.savedNode.isActive = true;
                this.savedNode.svg = svg;
            },
            updateSelection: function(self) {
                if (this.savedNode.hasOwnProperty('svg')) {
                    var fill = self.fills[self.metric];
                    this.savedNode.svg.style.strokeWidth = 2;
                    // this.savedNode.svg.attributes['filter'].value = "none"
                    this.savedNode.isActive = false;
                    this.savedNode.svg.attributes['stroke'].value = fill[fill.length - 1];
                }
            },
            resetSelection: function(self) {
                if (this.savedNode.hasOwnProperty('svg')) {
                    var fill = self.fills[self.metric];
                    // this.savedNode.svg.attributes['filter'].value = "none"
                    this.savedNode.isActive = false;
                    this.savedNode.svg.attributes['stroke'].value = fill[fill.length - 1];
                }
            },
            updateLegendUtilization: function() {
                var fill = this.fills[this.metric];
                for (var i = 0; i < fill.length; i++) {
                    var icon = $('#legendUtil' + i);
                    if (icon.children()) {
                        icon.children()[0].setAttribute('fill', fill[(fill.length - 1) - i]);
                    }
                }
                this.update();
            },
            newChart: function() {
                this.updateSelection(this);
                switch (this.chart) {
                    case 'radial':
                        this.buildRadial();
                        break;
                    case 'linear':
                        this.buildLinear();
                        break;
                }
            },
            update: function() {
                if (!this.colony) {
                    return;
                }
                if (this.refresh) {
                    this.refresh();
                    return;
                } else {
                    this.newChart();
                }
            },
            buildChildProperties: function(node) {
                if (node) {
                    node.children = this.sector.getChildren(node);
                    for (var i = 0; node.children && i < node.children.length; i++) {
                        // we stop diagram at vm level, change this to drill higher or lower
                        if (node.children[i].component_type != 'RWVM') {
                            // recurse
                            this.buildChildProperties(node.children[i]);
                        }
                    }
                }
            },
            nodeDim: function(v) {
                return {
                    textOffset: 20 * (v / this.maxSize),
                    circleRadius: 20 * (v / this.maxSize)
                };
            },
            graphs: {
                trafsim: 700,
                trianglePath: "M 0 0 l 30 20 l 0 -40 Z",
                stroke: function(d) {
                    if (!d.isActive) {
                        var fill = this.fills[this.metric];
                        return fill[fill.length - 1];
                    } else {
                        return "hsla(198, 83%, 61%, 1.0)";
                    }
                },
                fill: function(d) {
                    var fill = this.fills[this.metric];
                    var pct = this.utilization[this.metric](d);
                    var quartile = Math.min(fill.length - 1, Math.floor(fill.length * (pct / 100)));
                    return fill[quartile];
                },
                diagram: function(type, trafsim) {
                    function zoomed() {
                        diagram.attr("transform", "translate(" + d3.event.translate + ")scale(" + d3.event.scale + ")");
                    }
                    var translate = '';
                    switch (type) {
                        case 'radial':
                            translate = $('#diagram')[0].scrollWidth / 2 + "," + trafsim / 2;
                            break;
                        case 'linear':
                            translate = "40,0";
                            break;
                    }
                    var zoom = d3.behavior.zoom().scaleExtent([1, 5]).on("zoom", zoomed)
                    var drag = d3.behavior.drag().origin(function(d) {
                        return d;
                    }).on("dragstart", dragstarted).on("drag", dragged).on("dragend", dragended);
                    var container = d3.select('#diagram').attr("width", "100%").attr("height", "100%").append('g').call(zoom).call(drag);
                    var rect = container.append("rect").attr("width", "100%").attr("height", "100%").style("fill", "none").style("pointer-events", "all");
                    var diagram = container.append("g").append("g").attr("transform", "translate(" + translate + ")");
                    return diagram;
                },
                node: function(diagram, nodes, type) {
                    var translate;
                    switch (type) {
                        case 'radial':
                            translate = function(d) {
                                return "rotate(" + (d.x - 90) + ")translate(" + d.y + ")";
                            };
                            break;
                        case 'linear':
                            translate = function(d) {
                                return "translate(" + d.y + "," + d.x + ")";
                            };
                            break;
                    }
                    var node = diagram.selectAll(".node").data(nodes);
                    node.enter().append("g").attr("class", "node").attr("transform", translate)
                    return node;
                },
                square: function(node, self, fill) {
                    var graphs = this;
                    var square = node.selectAll(".colonyNode").data(function(d) {
                        return (d.component_type === 'RWCOLLECTION' && rw.vcs.nodeType(d) != 'rwcluster' ? [d] : []);
                    })
                    square.enter().append("rect").attr('class', 'colonyNode').attr('id', 'rootColony').attr('y', -15).attr('x', 0).attr('height', 30).attr('width', 30).on("click", function(d) {
                        self.nodeClick.call(this, d, self)
                    })
                    square.exit().remove();
                    square.attr('stroke', function(d) {
                        return graphs.stroke.call(self, d)
                    }).attr('fill', fill)
                    return square
                },
                triangle: function(node, self, fill) {
                    var graphs = this;
                    var triangle = node.selectAll(".clusterNode").data(function(d) {
                        return (rw.vcs.nodeType(d) === 'rwcluster' ? [d] : []);
                    });
                    triangle.enter().append("path").attr('d', graphs.trianglePath).attr('class', 'clusterNode').on("click", function(d) {
                        self.nodeClick.call(this, d, self)
                    });
                    triangle.exit().remove();
                    triangle.transition(50).attr('stroke', function(d) {
                        return graphs.stroke.call(self, d)
                    }).attr('fill', fill)
                    return triangle
                },
                circle: function(node, self, fill) {
                    var graphs = this;
                    var circle = node.selectAll(".vmNode").data(function(d) {
                        return (d.component_type == 'RWVM' ? [d] : []);
                    });
                    circle.enter().append("circle").attr("class", "vmNode").on("click", function(d) {
                        self.nodeClick.call(this, d, self)
                    });
                    circle.exit().remove();
                    circle.attr('stroke', function(d) {
                        return graphs.stroke.call(self, d)
                    }).transition(50).attr("r", function(d) {
                        var radius;
                        if (d.component_type == 'RWVM') {
                            var c = self.capacity[self.metric](d);
                            radius = Math.max(10, self.nodeRadius * c);
                        } else {
                            radius = self.nodeRadius;
                        }
                        return radius;
                    }).transition(50).attr("fill", fill)
                    return circle
                },
                link: function(diagram, links, diagonal) {
                    return diagram.selectAll(".link").data(links).enter().append("path").attr("class", "link").attr("d", diagonal);
                },
                text: function(self, node, type) {
                    var anchor, translate;
                    switch (type) {
                        case 'radial':
                            anchor = function(d) {
                                return d.x < 180
                            };
                            translate = 'rotate(180)';
                            break;
                        case 'linear':
                            anchor = function(d) {
                                return d.x
                            };
                            translate = ' ';
                    }
                    var txt = node.selectAll('.node-label').data(function(d) {
                        return [d];
                    });
                    txt.enter().append("text").attr('class', 'node-label').attr("dy", ".31em").attr("text-anchor", function(d) {
                        return anchor(d) ? "start" : "end";
                    }).text(function(d) {
                        return d.instance_name;
                    });
                    txt.exit().remove();
                    txt.transition(50).attr("transform", function(d) {
                        var textOffset;
                        if (d.component_type == 'RWVM') {
                            var c = self.capacity[self.metric](d);
                            textOffset = 4 + Math.max(10, self.nodeRadius * c);
                        } else {
                            textOffset = 32;
                        }
                        return anchor(d) ? "translate(" + textOffset + ")" : translate + " translate(-" + textOffset + ")";
                    })
                    return txt;
                },
                refresh: function(diagram, nodes, self, fill, type) {
                    var g = this;
                    return function() {
                        var node = g.node(diagram, nodes, type)
                        g.triangle(node, self, fill);
                        g.square(node, self, fill);
                        g.circle(node, self, fill);
                        g.text(self, node, type)
                    }
                }
            },
            nodeClick: function(node, context) {
                var svg = this;
                context.selectNode(node, svg);
            },
            buildLinear: function(callback) {
                var self = this;
                var trafsim = this.graphs.trafsim;
                this.buildChildProperties(this.colony);
                var tree = d3.layout.tree().size([trafsim, trafsim - 120])
                var diagonal = d3.svg.diagonal().projection(function(d) {
                    return [d.y, d.x];
                });
                var diagram = this.graphs.diagram.call(this, 'linear', trafsim)
                var nodes = tree.nodes(this.colony);
                var links = tree.links(nodes);
                var link = self.graphs.link(diagram, links, diagonal);
                var fill = self.graphs.fill.bind(this)
                var nodeClick = this.nodeClick;
                // give d3 a chance to render before selecting a node
                this.theAsync();
                this.refresh = this.graphs.refresh(diagram, nodes, self, fill, 'linear')
                this.refresh();
                d3.select(self.frameElement).style("height", trafsim - 150 + "px");
            },
            buildRadial: function() {
                var self = this;
                var trafsim = this.graphs.trafsim;;
                this.buildChildProperties(this.colony);
                var tree = d3.layout.tree().size([360, trafsim / 2 - 25]).separation(function(a, b) {
                    return (a.parent == b.parent ? 1 : 2) / a.depth;
                });
                var diagonal = d3.svg.diagonal.radial().projection(function(d) {
                    return [d.y, d.x / 180 * Math.PI];
                });
                var diagram = this.graphs.diagram.call(this, 'radial', trafsim)
                var nodes = tree.nodes(this.colony);
                var links = tree.links(nodes);
                var link = this.graphs.link(diagram, links, diagonal);
                var fill = this.graphs.fill.bind(this);
                var nodeClick = this.nodeClick;
                // give d3 a chance to render before selecting a node
                this.theAsync();
                this.refresh = this.graphs.refresh(diagram, nodes, self, fill, 'radial')
                this.refresh();
                d3.select(self.frameElement).style("height", trafsim - 150 + "px");
            }
        });
        return {
            restrict: 'AE',
            templateUrl: '/modules/views/rw.cmdcntr-resources_diagram.tmpl.html',
            scope: {
                sector: '='
            },
            replace: true,
            controller: Controller,
            controllerAs: 'resourcesDiagram',
            bindToController: true
        }
    };
})(window, window.angular);
