//Ported from rw-resources-model
module.exports = angular.module('cmdcntr')
.factory('vcsResourcesFactory', ['radio', vcsResourcesFactory]);

function vcsResourcesFactory(radio) {
    var appChannel = radio.channel('appChannel');
    return {
        vms: [],
        attached: function(type, data) {
            switch (type) {
                case 'services':
                    this.services = data;
                    this.servicesChanged();
                    break;
                case 'sector':
                    this.sector = data;
                    this.sectorChanged();
                    break;
                case 'vms':
                    this.vms = data;
                    this.vmsChanged();
                    break;
                default:
                    break
            }
        },
        servicesChanged: function() {
            this.vms = _.flatten(jsonPath.eval(this.services, '$[*].vm'));
            this.vmsChanged();
        },
        sectorChanged: function() {
            this.vms = this.sector.allVms();
            this.vmsChanged();
        },
        vmsChanged: function() {
            if (this.vms.length > 0) {
                this.update();
            }
        },
        update: function() {
            this.socket = new rw.api.SocketSubscriber('web/get');
            var url = '/api/operational/vcs/resources?deep';
            this.socket.websubscribe(url, this.load.bind(this), this.offline.bind(this));
        },
        aggregateAndPublish: function() {
            var self = this;
            if (this.sector) {
                this.sector.allClusters().forEach(function(cluster) {
                    self.aggregateVms(cluster, rw.vcs.vms(cluster));
                });
                this.sector.allColonies().forEach(function(colony) {
                    self.aggregateVms(colony, rw.vcs.vms(colony));
                });
                this.aggregateVms(this.sector, this.sector.allVms());
            } else if (this.services) {
                this.services.forEach(function(service) {
                    self.aggregateVms(service, service.vms);
                });
            }
            appChannel.trigger('vcs-resources-updated');
        },
        aggregateVms: function(o, vms) {
            var self = this;
            var sums = [
                ['cpu', 'ncores'],
                ['memory', 'total'],
                ['memory', 'curr-usage'],
                ['storage', 'total'],
                ['storage', 'curr-usage']
            ];
            sums.forEach(function(sum, i) {
                var total = 0;
                vms.forEach(function(vm, j) {
                    total += self.getProperty(vm, sum);
                });
                self.setProperty(o, sum, total);
            });
            var pcts = [
                ['cpu', 'percent'],
                ['cpu', 'aggregate', 'system'],
                ['cpu', 'aggregate', 'user'],
                ['cpu', 'aggregate', 'idle'],
                ['storage', 'percent'],
                ['memory', 'percent']
            ];
            pcts.forEach(function(pct, i) {
                var total = 0;
                vms.forEach(function(vm, j) {
                    total += self.getProperty(vm, pct);
                });
                self.setProperty(o, pct, Math.round(total / vms.length));
            });
        },
        getProperty: function(o, path) {
            var x = o;
            for (var i = 0; i < path.length; i++) {
                x = x[path[i]];
            }
            return parseInt(x);
        },
        setProperty: function(o, path, value) {
            var x = o;
            for (var i = 0; i < path.length - 1; i++) {
                if (!(path[i] in x)) {
                    x[path[i]] = {};
                }
                x = x[path[i]];
            }
            var last = path[path.length - 1];
            x[last] = value;
        },
        offline: function(data) {
            var self = this;
            if (!('offline' in data)) {
                this.load(data);
                data.offline = true;
                return;
            }
            this.vms.forEach(function(vm) {
                // makeup metrics here
                self.offlineVm(vm);
            });
            this.aggregateAndPublish();
        },
        offlineVm: function(vm) {
            var rate = rw.trafgen.ratePerceived;
            if (rate == 0) {
                rate = rw.trafsim.ratePerceived;
            }
            var ratePct = (rate / 100);
            vm.cpu.aggregate.idle = 100 - rate;
            vm.cpu.aggregate.system = (100 - vm.cpu.aggregate.idle) / 2;
            vm.cpu.aggregate.user = (100 - vm.cpu.aggregate.idle) / 2;
            this.offlineUtilization(vm.memory, 'curr-usage', vm.memory.total, .01 + ratePct * .09);
            this.offlineUtilization(vm.storage, 'curr-usage', vm.storage.total, .005 + ratePct * .05);
            this.elaborateData(vm);
        },
        offlineUtilization: function(o, prop, capacity, variance) {
            var u = parseInt(o[prop]) || Math.random() * (capacity / 2);
            u += (.5 - Math.random()) * variance * capacity;
            u = Math.round(Math.min(Math.max(u, 0), capacity));
            o[prop] = u;
        },
        load: function(data) {
            var self = this;
            var metrics = _.flatten(jsonPath.eval(data, '$..vm'));
            this.vms.forEach(function(vm, i) {
                metrics.forEach(function(metric, j) {
                    if (metric.id == vm.instance_id) {
                        _.extend(vm, metric);
                        self.elaborateData(vm);
                        if (vm.process && metric.processes) {
                            self.loadProcessMetrics(vm, vm.process, metric.processes);
                        }
                        return false;
                    }
                });
            });
            this.aggregateAndPublish();
        },
        loadProcessMetrics: function(vm, processes, metrics) {
            var self = this;
            processes.forEach(function(process) {
                metrics.forEach(function(metric) {
                    if (process.instance_name == metric['instance-name']) {
                        _.extend(process, metric);
                        process.cpuPercent = Number(process.cpu).toFixed(2);
                        process.memoryPercent = self.pct(process.memory, vm.memory.total);
                        return false;
                    }
                });
            });
        },
        elaborateData: function(vm) {
            vm.cpu.percent = Math.min(100, Math.round(100 - parseInt(vm.cpu.aggregate.idle)));
            vm.cpu.ncores = vm.cpu.individual.length;
            vm.memory.percent = this.pct(vm.memory['curr-usage'], vm.memory.total);
            vm.storage.percent = this.pct(vm.storage['curr-usage'], vm.storage.total);
        },
        pct: function(usedStr, totalStr) {
            var used = parseInt(usedStr);
            var total = parseInt(totalStr);
            if (isNaN(used) || isNaN(total)) {
                return 0;
            }
            return Math.round(100 * (used / total));
        },
        unsubscribe: function() {
            if (this.socket) {
                this.socket.unsubscribe();
                this.socket = null;
            }
        },
        detached: function() {
            this.unsubscribe();
        }
    }
}
