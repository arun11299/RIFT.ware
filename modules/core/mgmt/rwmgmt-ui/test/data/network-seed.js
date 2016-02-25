function seedVeryBasic() {
  return [{
    id: 4,
    name: "traffic generator",
    type: "TRAFGEN",
    ifaces : [{
      id: 10,
      name: 'tg1.1',
      context: "Switch 1",
      destinations: [ {id:11} ],
      status : "OK",
      ipv4Address : "10.21.12.43",
      description: "awesome interface group",
      ports: [{
        id : "000:000:12:12",
        name: 'eth_sim:name=fabric',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-05",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
       },{
        id : "000:000:12:15",
        name: 'eth_ring:name=test1',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-07",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      }]
    }]
  },{
    id: 5,
    name: "load balancer",
    type: "LOADBAL",
    ifaces : [{
      id: 11,
      context: "Switch 1",
      status : "OK",
      ipv4Address : "10.21.99.43",
      description: "awesome interface group",
      ports: [{
        id : "000:000:12:12",
        name: 'lb1.0',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-05",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      },{
        id : "000:000:12:14",
        name: 'lb1.1',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-06",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      },{
        id : "000:000:12:15",
        name: 'lb1.2',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-07",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      }]
    },{
      id: 12,
      context: "Switch 2",
      destinations: [ {id:13} ],
      ipv4Address : "10.21.0.13",
      description: "awesome interface group",
      ports: [{
        id : "000:000:12:12",
        name: 'lb1b.0',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-05",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      },{
        id : "000:000:12:14",
        name: 'lb1b.1',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-06",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      },{
        id : "000:000:12:15",
        name: 'lb1b.2',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-07",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      }]
    }]
  },{
    id: 6,
    name: "traffic sink",
    type: "TRAFSINK",
    ifaces : [{
      id : 13,
      name: "ts1.1",
      context: "Switch 2",
      status : "OK",
      ipv4Address : "10.21.12.43",
      description: "awesome interface group",
      ports: [{
        id : "000:000:12:12",
        name: 'ts1.0',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-05",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      },{
        id : "000:000:12:14",
        name: 'ts1.1',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-06",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      },{
        id : "000:000:12:15",
        name: 'ts1.2',
        linkStatus : "OK",
        adminStatus : "OK",
        macAddress : "1D-0F-A5-31-EB-07",
        capacity : 1000,
        txFlowControl: true,
        rxFlowControl: false
      }]
    }]
  }];
}

function seedBasicIfaces() {
  return [{
    id: 1,
    label: "trafgen",
    type: "TRAFGEN",
    groups : [{
      id: 1,
      context: "Switch 1",
      destGroups: [ 2, 22 ],
      ifaces : [{
        id: 1,
        label: "tg1.1",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
         },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 2,
        label: "tg1.2",
        status : "OK",
        ipv4Address : "10.21.12.45",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }, {
        id: 21,
        label: "tg1.21",
        status : "OK",
        ipv4Address : "10.21.12.47",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 22,
        label: "tg1.22",
        status : "OK",
        ipv4Address : "10.21.12.49",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    }]
  },{
    id: 2,
    label: "loadbal",
    type: "LOADBAL",
    groups : [{
      id: 2,
      context: "Switch 1",
      ifaces : [{
        id: 3,
        context: "Switch 1",
        label: "lb1.1",
        status : "OK",
        ipv4Address : "10.21.99.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 4,
        context: "Switch 1",
        label: "lb1.2",
        status : "OK",
        ipv4Address : "10.21.99.41",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    },{
      id: 3,
      context: "Switch 2",
      destGroups: [ 4, 10 ],
      ifaces : [{
        id: 5,
        label: "lb1.1",
        status : "OK",
        ipv4Address : "10.21.0.13",
        description: "awesome interface group",
        ports: [{
         id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 6,
        label: "lb1.2",
        status : "OK",
        ipv4Address : "10.21.0.15",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    }]
  },{
    id: 22,
    label: "loadbal2",
    type: "LOADBAL",
    groups : [{
      id: 22,
      context: "Switch 1",
      ifaces : [{
        id: 23,
        context: "Switch 1",
        label: "lb1.1",
        status : "OK",
        ipv4Address : "10.21.99.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 24,
        context: "Switch 1",
        label: "lb1.2",
        status : "OK",
        ipv4Address : "10.21.99.41",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    },{
      id: 23,
      context: "Switch 2",
      destGroups: [ 12, 14 ],
      ifaces : [{
        id: 25,
        label: "lb1.1",
        status : "OK",
        ipv4Address : "10.21.0.13",
        description: "awesome interface group",
        ports: [{
         id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 26,
        label: "lb1.2",
        status : "OK",
        ipv4Address : "10.21.0.15",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    }]
  },{
    id: 3,
    label: "echo app",
    type: "TRAFSINK",
    groups : [{
      id : 4,
      context: "Switch 2",
      ifaces : [{
        id: 7,
        label: "ts1.1",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 8,
        label: "ts1.2",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    }]
  },{
    id: 4,
    label: "echo app",
    type: "TRAFSINK",
    groups : [{
      id : 10,
      context: "Switch 2",
      ifaces : [{
        id: 7,
        label: "ts1.1",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 11,
        label: "ts1.2",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    }]
  },{
    id: 5,
    label: "echo app",
    type: "TRAFSINK",
    groups : [{
      id : 12,
      context: "Switch 2",
      ifaces : [{
        id: 7,
        label: "ts1.1",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 13,
        label: "ts1.2",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    }]
  },{
    id: 6,
    label: "echo app",
    type: "TRAFSINK",
    groups : [{
      id : 14,
      context: "Switch 2",
      ifaces : [{
        id: 7,
        label: "ts1.1",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      },{
        id: 15,
        label: "ts1.2",
        status : "OK",
        ipv4Address : "10.21.12.43",
        description: "awesome interface group",
        ports: [{
          id : "000:000:12:12",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-05",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:14",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-06",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        },{
          id : "000:000:12:15",
          linkStatus : "OK",
          adminStatus : "OK",
          macAddress : "1D-0F-A5-31-EB-07",
          capacity : 1000,
          txFlowControl: true,
          rxFlowControl: false
        }]
      }]
    }]
  }];
}