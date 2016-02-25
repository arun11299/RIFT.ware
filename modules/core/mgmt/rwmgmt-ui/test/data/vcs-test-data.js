function basicColony() {
  return [{
    instance_name : 'colony',
    instance_id : '1',
    clusters : [{
      instance_id : '1',
      component_type : 'RWCOLONY',
      instance_name: 'cluster1',
      label: 'cluster',
      vms : [{
        component_type : 'RWVM',
        instance_name: 'vm1',
        instance_id : '1',
        tasklets: [ {
          component_type : 'RWTASKLET',
          instance_name: 'tasklet1',
          instance_id : '1'
        }]
      }]
    }]
  }];
}

function generatePorts(nPorts) {
  var ports = [];
  for (var i = 0; i < nPorts; i++) {
    ports.push({
      id: '000:000:000:' + i,
      state: 'up',
      adminstate: 'up',
      mac: '1D-OF-A5-31-EB-' + i,
      speed: 1000,
      txFlow: 'on',
      rxFlow: 'on',
      iface: 'iface-1'
    });
  }
  return ports;
}

function randomColony(nClusters, nVms, nProcesses, nTasklets, deviation) {
  return _customColony(nClusters, nVms, nProcesses, nTasklets, deviation);
}

function customColony(nClusters, nVms, nProcesses, nTasklets) {
  return _customColony(nClusters, nVms, nProcesses, nTasklets, 0);
}

function randomInt(low, high) {
  var range = high - low;
  return Math.min(high, low + Math.floor(Math.random() * (range + 1)));
}

function _customColony(nClustersIn, nVmsIn, nProcessesIn, nTaskletsIn, deviation) {
  var colony = {
    instance_name : 'colony',
    instance_id : '1',
    component_type : 'RWCOLONY',
    clusters : [],
    state: 'up'
  };

  var nClusters = deviation ? randomInt(nClustersIn - deviation, nClustersIn + deviation) : nClustersIn;
  for (var i = 1; i <= nClusters; i++) {
    var cluster = {
      instance_id : '1',
      component_type : 'RWCLUSTER',
      instance_name: 'cluster-' + i,
      label: 'cluster',
      vms : [],
      state: 'up'
    };
    colony.clusters.push(cluster);
    var nVms = deviation ? randomInt(nVmsIn - deviation, nVmsIn + deviation) : nVmsIn;
    for (var j = 1; j <= nVms; j++) {
      var vm = {
        component_type : 'RWVM',
        instance_name: 'vm-' + (i * j),
        instance_id : '' + (i * j),
        processes: [],
        ports: generatePorts(2),
        state: 'up'
      };
      cluster.vms.push(vm);
      var nProcesses = deviation ? randomInt(nProcessesIn - deviation, nProcessesIn + deviation) : nProcessesIn;
      for (var k = 1; k <= nProcesses; k++) {
        var process = {
          component_type : 'RWPROCESS',
          instance_name: 'process-' + (i * j * k),
          instance_id : '' + (i * j * k),
          tasklets: [],
          state: 'running',
          file_descriptors: Math.round(16 * Math.random()),
          cpu_utilization: Math.round(50 * Math.random()),
          mem_utilization: Math.round(50 * Math.random()),
          pid: Math.round(65536 * Math.random())
        };
        vm.processes.push(process);
        var nTasklets = deviation ? randomInt(nTaskletsIn - deviation, nTaskletsIn + deviation) : nTaskletsIn;
        for (var l = 1; l <= nTasklets; l++) {
          var tasklet = {
            component_type : 'RWTASKLET',
            process : process,
            state: 'running',
            instance_name: 'tasklet-' + (i * j * k * l),
            instance_id : '' + (i * j * k * l),
            mem_utilization: Math.round(25 * Math.random()),
            thread_count : Math.round(16 * Math.random()),
            file_descriptors : Math.round(16 * Math.random()),
            scheduler_queues : Math.round(16 * Math.random())
          };
          process.tasklets.push(tasklet);
        }
      }
    }
  }
  return colony;
}

function smallColony() {
  return {
    instance_name : 'colony',
    instance_id : '1',
    clusters : [{
      instance_id : '1',
      component_type : 'RWCOLONY',
      instance_name: 'cluster1',
      label: 'cluster',
      vms : [{
        component_type : 'RWVM',
        instance_name: 'vm1',
        instance_id : '1',
        tasklets: [ {
          component_type : 'RWTASKLET',
          instance_name: 'tasklet1',
          instance_id : '1'
        }]
      }]
    },{
      instance_id : '2',
      component_type : 'RWCOLONY',
      instance_name: 'cluster2',
      label: 'cluster',
      vms : [{
        component_type : 'RWVM',
        instance_name: 'vm2',
        instance_id : '2',
        tasklets: [ {
          component_type : 'RWTASKLET',
          instance_name: 'tasklet2',
          instance_id : '2'
        }]
      }]
    },{
      instance_id : '2',
      component_type : 'RWCOLONY',
      instance_name: 'cluster2',
      label: 'cluster',
      vms : [{
        component_type : 'RWVM',
        instance_name: 'vm2',
        instance_id : '2',
        tasklets: [ {
          component_type : 'RWTASKLET',
          instance_name: 'tasklet2',
          instance_id : '2'
        }]
      }]
    }]
  };
}