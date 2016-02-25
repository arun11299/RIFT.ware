function offlineScriptStart($scope, self, $rootScope, ueScript){
  self.scriptState = {script: {}};
  self.imsiState = {state:{}};
  setTimeout(function(){
    self.scriptState.script.state = "Active";
    if(!ueScript.offlineRunning) {
      self.imsiState.state.mobility = "Idle";
      self.scriptState.script['duration-secs'] = 0;
    } else{
      self.scriptState.script['duration-secs'] = Math.round(Math.random() * (100 - 10) + 10);
    }
    self.script_running = 2;
    ueScript.offlineRunning = true;
    $scope.$apply();

    setTimeout(function(){
      self.scriptState.calls = {
      "success": 1,
        "failure": 0,
        "total": 1,
        "active": 1
      };

      self.imsiState = offlineImsi;
      self.logs = 'testing stuff'
      self.imsiState.state.mobility = "Connected";
      console.log('applying', self.imsiState);
      $rootScope.$broadcast('ue.script.running', {logs: offlineLogs});
      //
      $scope.$apply();
    },1000);

  },1000)
}


function offlineScriptStop($scope, self, $rootScope){
  setTimeout(function(){
    self.scriptState.script.state = 'Terminated';
    self.script_running = 0;
    self.offlineRunning=false;
    $scope.$apply();
  },1000);

}

var offlineImsi = {
  "imsi": "123456789012345",
  "imei": "12345678901234",
  "msisdn": "1122334556667",
  "state": {
    "mobility": "Connected",
    "data-transfer": "Unknown"
  },
  "group-id": 9,
  "pdn-ipv4-address": "172.16.12.254",
  "pdn-ipv6-address": "2345:40:40:fffe::193:5de8",
  "serving-network": {
    "mcc": 123,
    "mnc": 456
  },
  "ue-ambr": {
    "uplink": 4096,
    "downlink": 4096
  },
  "mme": "11.0.1.4",
  "sgw": "11.0.1.3",
  "uplink-pkts": 1828,
  "uplink-bytes": 171832,
  "downlink-pkts": 1828,
  "downlink-bytes": 171832,
  "dns": {
    "ipv4": [
      {
        "addr": "1.1.1.1"
      },
      {
        "addr": "2.2.2.2"
      }
    ],
    "ipv6": [
      {
        "addr": "abcd::40:40:1"
      },
      {
        "addr": "abcd::40:40:2"
      }
    ]
  },
  "pdn-connections": [
    {
      "apn": "riftio.com.mnc012.mcc345.gprs",
      "pdn-type": "IPv4v6",
      "pgw": "11.0.1.3",
      "bearers": [
        {
          "id": 5,
          "pgw-teid": 26436756,
          "enb-teid": 8673256,
          "sgw-teid": 26436728,
          "qos": {
            "qci": 5,
            "arp-pvi": false,
            "arp-pl": 2,
            "arp-pci": false
          }
        }
      ]
    }
  ]
};

var offlineLogs = [{
  "msg-id": "001421605566.395438/rwopenstack-grunt23-vm32/7946/7946/000000001171",
  "msg": "2015-01-18T18:26:06.395438Z rwopenstack-grunt23-vm32::info[rwltesim_ue.py-7946-7946] version:1.0 rwltesim_ue.py:280 notification:rw-appmgr-log.ue-script-start event-id:20401 groupcallid:1 imei:12345678901234 imsi:123456789012345 msisdn:1122334556667 script_id:ping_dns_continuous sequence:1171"
},
  {
    "msg-id": "001421605576.410307/rwopenstack-grunt23-vm32/8414/8414/000000000001",
    "msg": "2015-01-18T18:26:16.410307Z rwopenstack-grunt23-vm32::info[session.py-8414-8414] version:1.0 session.py:277 notification:rw-appmgr-log.ue-script-creating-session event-id:20201 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 imei:12345678901234 imsi:123456789012345 msisdn:1122334556667 pgw_ip:15.0.0.2 sequence:1 sgw_ip:11.0.1.3"
  },
  {
    "msg-id": "001421605576.496383/rwopenstack-grunt23-vm32/8112/8112/000000000016",
    "msg": "2015-01-18T18:26:16.496383Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwMmeClientApi.c:175 notification:rw-appmgr-log.nwepc-create-ue-session-received event-id:20110 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 imsi:123456789012345 msg:Create UE session REST Command received pgw_ip:15.0.0.2 sequence:16 sgw_ip:11.0.1.3"
  },
  {
    "msg-id": "001421605577.197397/rwopenstack-grunt23-vm32/8112/8112/000000000028",
    "msg": "2015-01-18T18:26:17.197397Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:28"
  },
  {
    "msg-id": "001421605577.197525/rwopenstack-grunt23-vm32/8112/8112/000000000034",
    "msg": "2015-01-18T18:26:17.197525Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwMmeUe.c:813 notification:rw-appmgr-log.nwepc-session-message-sent event-id:20111 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 gtpc_msg:gtp_create_session_req imsi:123456789012345 msg:Create Session Request sent peer_ip:11.0.1.3 sequence:34"
  },
  {
    "msg-id": "001421605577.200352/rwopenstack-grunt23-vm32/8112/8112/000000000037",
    "msg": "2015-01-18T18:26:17.200352Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:37"
  },
  {
    "msg-id": "001421605577.200562/rwopenstack-grunt23-vm32/8112/8112/000000000059",
    "msg": "2015-01-18T18:26:17.200562Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwMmeUe.c:1041 notification:rw-appmgr-log.nwepc-session-message-received event-id:20112 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 gtpc_msg:gtp_create_session_rsp imsi:123456789012345 msg:Create Session Response received peer_ip:11.0.1.3 sequence:59"
  },
  {
    "msg-id": "001421605577.201390/rwopenstack-grunt23-vm32/8112/8112/000000000070",
    "msg": "2015-01-18T18:26:17.201390Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwMmeUe.c:1110 notification:rw-appmgr-log.nwepc-create-session-response-success event-id:20114 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 imsi:123456789012345 msg:UE Session GTPv2C Create Session Response success peer_ip:11.0.1.3 sequence:70"
  },
  {
    "msg-id": "001421605577.236486/rwopenstack-grunt23-vm32/8112/8112/000000000103",
    "msg": "2015-01-18T18:26:17.236486Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:103"
  },
  {
    "msg-id": "001421605577.240419/rwopenstack-grunt23-vm32/8112/8112/000000000109",
    "msg": "2015-01-18T18:26:17.240419Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwMmeUe.c:372 notification:rw-appmgr-log.nwepc-session-message-sent event-id:20111 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 gtpc_msg:gtp_modify_bearer_req imsi:123456789012345 msg:Modify Bearer Request sent peer_ip:11.0.1.3 sequence:109"
  },
  {
    "msg-id": "001421605577.240480/rwopenstack-grunt23-vm32/8112/8112/000000000112",
    "msg": "2015-01-18T18:26:17.240480Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:112"
  },
  {
    "msg-id": "001421605577.240590/rwopenstack-grunt23-vm32/8112/8112/000000000125",
    "msg": "2015-01-18T18:26:17.240590Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwMmeUe.c:1422 notification:rw-appmgr-log.nwepc-session-message-received event-id:20112 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 gtpc_msg:gtp_modify_bearer_rsp imsi:123456789012345 msg:Modify Beaerer Response received peer_ip:11.0.1.3 sequence:125"
  },
  {
    "msg-id": "001421605577.240610/rwopenstack-grunt23-vm32/8112/8112/000000000126",
    "msg": "2015-01-18T18:26:17.240610Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwMmeUe.c:1444 notification:rw-appmgr-log.nwepc-ue-session-establish-success event-id:20117 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 imsi:123456789012345 msg:UE Session Established successfully pdn_type:IPv4v6 peer_ip:11.0.1.3 sequence:126 ue_ipv4_addr:172.16.12.254 ue_ipv6_addr:2345:40:40:fffe::24f:5de8"
  },
  {
    "msg-id": "001421605578.679544/rwopenstack-grunt23-vm32/8414/8414/000000000013",
    "msg": "2015-01-18T18:26:18.679544Z rwopenstack-grunt23-vm32::info[session.py-8414-8414] version:1.0 session.py:329 notification:rw-appmgr-log.ue-script-session-created event-id:20205 apn:riftio.com.mnc012.mcc345.gprs groupcallid:1 imei:12345678901234 imsi:123456789012345 msisdn:1122334556667 netns_name:RwUESim-0 pgw_ip:15.0.0.2 sequence:13 sgw_ip:11.0.1.3"
  },
  {
    "msg-id": "001421605579.520556/rwopenstack-grunt23-vm32/8414/8414/000000000017",
    "msg": "2015-01-18T18:26:19.520556Z rwopenstack-grunt23-vm32::info[session.py-8414-8414] version:1.0 session.py:331 notification:rw-logger-log.rwlogger-log-info event-id:18001 groupcallid:1 sequence:17"
  },
  {
    "msg-id": "001421605579.526379/rwopenstack-grunt23-vm32/8414/8414/000000000018",
    "msg": "2015-01-18T18:26:19.526379Z rwopenstack-grunt23-vm32::info[session.py-8414-8414] version:1.0 session.py:231 notification:rw-logger-log.rwlogger-log-info event-id:18001 groupcallid:1 log:Continuously pinging all DNS interfaces every second until killed. sequence:18"
  },
  {
    "msg-id": "001421605579.534292/rwopenstack-grunt23-vm32/8414/8414/000000000019",
    "msg": "2015-01-18T18:26:19.534292Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (1.1.1.1) using command: ping -c 1 -W 2 1.1.1.1 sequence:19"
  },
  {
    "msg-id": "001421605579.659320/rwopenstack-grunt23-vm32/8112/8112/000000000137",
    "msg": "2015-01-18T18:26:19.659320Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:137"
  },
  {
    "msg-id": "001421605579.659612/rwopenstack-grunt23-vm32/8112/8112/000000000142",
    "msg": "2015-01-18T18:26:19.659612Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:142"
  },
  {
    "msg-id": "001421605579.664352/rwopenstack-grunt23-vm32/8414/8414/000000000020",
    "msg": "2015-01-18T18:26:19.664352Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (2.2.2.2) using command: ping -c 1 -W 2 2.2.2.2 sequence:20"
  },
  {
    "msg-id": "001421605579.763444/rwopenstack-grunt23-vm32/8112/8112/000000000150",
    "msg": "2015-01-18T18:26:19.763444Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:150"
  },
  {
    "msg-id": "001421605579.764292/rwopenstack-grunt23-vm32/8112/8112/000000000155",
    "msg": "2015-01-18T18:26:19.764292Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:155"
  },
  {
    "msg-id": "001421605579.767475/rwopenstack-grunt23-vm32/8414/8414/000000000021",
    "msg": "2015-01-18T18:26:19.767475Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:1) using command: ping6 -c 1 -W 2 abcd::40:40:1 sequence:21"
  },
  {
    "msg-id": "001421605579.854390/rwopenstack-grunt23-vm32/8112/8112/000000000163",
    "msg": "2015-01-18T18:26:19.854390Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:163"
  },
  {
    "msg-id": "001421605579.856357/rwopenstack-grunt23-vm32/8112/8112/000000000168",
    "msg": "2015-01-18T18:26:19.856357Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:168"
  },
  {
    "msg-id": "001421605579.859339/rwopenstack-grunt23-vm32/8414/8414/000000000022",
    "msg": "2015-01-18T18:26:19.859339Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:2) using command: ping6 -c 1 -W 2 abcd::40:40:2 sequence:22"
  },
  {
    "msg-id": "001421605579.941594/rwopenstack-grunt23-vm32/8112/8112/000000000176",
    "msg": "2015-01-18T18:26:19.941594Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:176"
  },
  {
    "msg-id": "001421605579.942482/rwopenstack-grunt23-vm32/8112/8112/000000000181",
    "msg": "2015-01-18T18:26:19.942482Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:181"
  },
  {
    "msg-id": "001421605580.949414/rwopenstack-grunt23-vm32/8414/8414/000000000023",
    "msg": "2015-01-18T18:26:20.949414Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (1.1.1.1) using command: ping -c 1 -W 2 1.1.1.1 sequence:23"
  },
  {
    "msg-id": "001421605581.006463/rwopenstack-grunt23-vm32/8112/8112/000000000189",
    "msg": "2015-01-18T18:26:21.006463Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:189"
  },
  {
    "msg-id": "001421605581.007328/rwopenstack-grunt23-vm32/8112/8112/000000000194",
    "msg": "2015-01-18T18:26:21.007328Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:194"
  },
  {
    "msg-id": "001421605581.009375/rwopenstack-grunt23-vm32/8414/8414/000000000024",
    "msg": "2015-01-18T18:26:21.009375Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (2.2.2.2) using command: ping -c 1 -W 2 2.2.2.2 sequence:24"
  },
  {
    "msg-id": "001421605581.059522/rwopenstack-grunt23-vm32/8112/8112/000000000202",
    "msg": "2015-01-18T18:26:21.059522Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:202"
  },
  {
    "msg-id": "001421605581.061276/rwopenstack-grunt23-vm32/8112/8112/000000000207",
    "msg": "2015-01-18T18:26:21.061276Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:207"
  },
  {
    "msg-id": "001421605581.063418/rwopenstack-grunt23-vm32/8414/8414/000000000025",
    "msg": "2015-01-18T18:26:21.063418Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:1) using command: ping6 -c 1 -W 2 abcd::40:40:1 sequence:25"
  },
  {
    "msg-id": "001421605581.115413/rwopenstack-grunt23-vm32/8112/8112/000000000215",
    "msg": "2015-01-18T18:26:21.115413Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:215"
  },
  {
    "msg-id": "001421605581.116339/rwopenstack-grunt23-vm32/8112/8112/000000000220",
    "msg": "2015-01-18T18:26:21.116339Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:220"
  },
  {
    "msg-id": "001421605581.119408/rwopenstack-grunt23-vm32/8414/8414/000000000026",
    "msg": "2015-01-18T18:26:21.119408Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:2) using command: ping6 -c 1 -W 2 abcd::40:40:2 sequence:26"
  },
  {
    "msg-id": "001421605581.169500/rwopenstack-grunt23-vm32/8112/8112/000000000228",
    "msg": "2015-01-18T18:26:21.169500Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:228"
  },
  {
    "msg-id": "001421605581.170332/rwopenstack-grunt23-vm32/8112/8112/000000000233",
    "msg": "2015-01-18T18:26:21.170332Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:233"
  },
  {
    "msg-id": "001421605582.173618/rwopenstack-grunt23-vm32/8414/8414/000000000027",
    "msg": "2015-01-18T18:26:22.173618Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (1.1.1.1) using command: ping -c 1 -W 2 1.1.1.1 sequence:27"
  },
  {
    "msg-id": "001421605582.235519/rwopenstack-grunt23-vm32/8112/8112/000000000241",
    "msg": "2015-01-18T18:26:22.235519Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:241"
  },
  {
    "msg-id": "001421605582.236505/rwopenstack-grunt23-vm32/8112/8112/000000000246",
    "msg": "2015-01-18T18:26:22.236505Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:246"
  },
  {
    "msg-id": "001421605582.239308/rwopenstack-grunt23-vm32/8414/8414/000000000028",
    "msg": "2015-01-18T18:26:22.239308Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (2.2.2.2) using command: ping -c 1 -W 2 2.2.2.2 sequence:28"
  },
  {
    "msg-id": "001421605582.284516/rwopenstack-grunt23-vm32/8112/8112/000000000254",
    "msg": "2015-01-18T18:26:22.284516Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:254"
  },
  {
    "msg-id": "001421605582.285474/rwopenstack-grunt23-vm32/8112/8112/000000000259",
    "msg": "2015-01-18T18:26:22.285474Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:259"
  },
  {
    "msg-id": "001421605582.287479/rwopenstack-grunt23-vm32/8414/8414/000000000029",
    "msg": "2015-01-18T18:26:22.287479Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:1) using command: ping6 -c 1 -W 2 abcd::40:40:1 sequence:29"
  },
  {
    "msg-id": "001421605582.335399/rwopenstack-grunt23-vm32/8112/8112/000000000267",
    "msg": "2015-01-18T18:26:22.335399Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:267"
  },
  {
    "msg-id": "001421605582.336350/rwopenstack-grunt23-vm32/8112/8112/000000000272",
    "msg": "2015-01-18T18:26:22.336350Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:272"
  },
  {
    "msg-id": "001421605582.338496/rwopenstack-grunt23-vm32/8414/8414/000000000030",
    "msg": "2015-01-18T18:26:22.338496Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:2) using command: ping6 -c 1 -W 2 abcd::40:40:2 sequence:30"
  },
  {
    "msg-id": "001421605582.390602/rwopenstack-grunt23-vm32/8112/8112/000000000280",
    "msg": "2015-01-18T18:26:22.390602Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:280"
  },
  {
    "msg-id": "001421605582.391517/rwopenstack-grunt23-vm32/8112/8112/000000000285",
    "msg": "2015-01-18T18:26:22.391517Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:285"
  },
  {
    "msg-id": "001421605583.395551/rwopenstack-grunt23-vm32/8414/8414/000000000031",
    "msg": "2015-01-18T18:26:23.395551Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (1.1.1.1) using command: ping -c 1 -W 2 1.1.1.1 sequence:31"
  },
  {
    "msg-id": "001421605583.463538/rwopenstack-grunt23-vm32/8112/8112/000000000293",
    "msg": "2015-01-18T18:26:23.463538Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:293"
  },
  {
    "msg-id": "001421605583.464449/rwopenstack-grunt23-vm32/8112/8112/000000000298",
    "msg": "2015-01-18T18:26:23.464449Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:298"
  },
  {
    "msg-id": "001421605583.467332/rwopenstack-grunt23-vm32/8414/8414/000000000032",
    "msg": "2015-01-18T18:26:23.467332Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (2.2.2.2) using command: ping -c 1 -W 2 2.2.2.2 sequence:32"
  },
  {
    "msg-id": "001421605583.515510/rwopenstack-grunt23-vm32/8112/8112/000000000306",
    "msg": "2015-01-18T18:26:23.515510Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:306"
  },
  {
    "msg-id": "001421605583.516436/rwopenstack-grunt23-vm32/8112/8112/000000000311",
    "msg": "2015-01-18T18:26:23.516436Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:311"
  },
  {
    "msg-id": "001421605583.519480/rwopenstack-grunt23-vm32/8414/8414/000000000033",
    "msg": "2015-01-18T18:26:23.519480Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:1) using command: ping6 -c 1 -W 2 abcd::40:40:1 sequence:33"
  },
  {
    "msg-id": "001421605583.579367/rwopenstack-grunt23-vm32/8112/8112/000000000319",
    "msg": "2015-01-18T18:26:23.579367Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:319"
  },
  {
    "msg-id": "001421605583.580405/rwopenstack-grunt23-vm32/8112/8112/000000000324",
    "msg": "2015-01-18T18:26:23.580405Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:324"
  },
  {
    "msg-id": "001421605583.583432/rwopenstack-grunt23-vm32/8414/8414/000000000034",
    "msg": "2015-01-18T18:26:23.583432Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:2) using command: ping6 -c 1 -W 2 abcd::40:40:2 sequence:34"
  },
  {
    "msg-id": "001421605583.632497/rwopenstack-grunt23-vm32/8112/8112/000000000332",
    "msg": "2015-01-18T18:26:23.632497Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:332"
  },
  {
    "msg-id": "001421605583.633508/rwopenstack-grunt23-vm32/8112/8112/000000000337",
    "msg": "2015-01-18T18:26:23.633508Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:337"
  },
  {
    "msg-id": "001421605584.637603/rwopenstack-grunt23-vm32/8414/8414/000000000035",
    "msg": "2015-01-18T18:26:24.637603Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (1.1.1.1) using command: ping -c 1 -W 2 1.1.1.1 sequence:35"
  },
  {
    "msg-id": "001421605584.688330/rwopenstack-grunt23-vm32/8112/8112/000000000345",
    "msg": "2015-01-18T18:26:24.688330Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:345"
  },
  {
    "msg-id": "001421605584.689321/rwopenstack-grunt23-vm32/8112/8112/000000000350",
    "msg": "2015-01-18T18:26:24.689321Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:350"
  },
  {
    "msg-id": "001421605584.691556/rwopenstack-grunt23-vm32/8414/8414/000000000036",
    "msg": "2015-01-18T18:26:24.691556Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (2.2.2.2) using command: ping -c 1 -W 2 2.2.2.2 sequence:36"
  },
  {
    "msg-id": "001421605584.736530/rwopenstack-grunt23-vm32/8112/8112/000000000358",
    "msg": "2015-01-18T18:26:24.736530Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:358"
  },
  {
    "msg-id": "001421605584.737441/rwopenstack-grunt23-vm32/8112/8112/000000000363",
    "msg": "2015-01-18T18:26:24.737441Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:363"
  },
  {
    "msg-id": "001421605584.740548/rwopenstack-grunt23-vm32/8414/8414/000000000037",
    "msg": "2015-01-18T18:26:24.740548Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:1) using command: ping6 -c 1 -W 2 abcd::40:40:1 sequence:37"
  },
  {
    "msg-id": "001421605584.828616/rwopenstack-grunt23-vm32/8112/8112/000000000371",
    "msg": "2015-01-18T18:26:24.828616Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:371"
  },
  {
    "msg-id": "001421605584.829493/rwopenstack-grunt23-vm32/8112/8112/000000000376",
    "msg": "2015-01-18T18:26:24.829493Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:376"
  },
  {
    "msg-id": "001421605584.834317/rwopenstack-grunt23-vm32/8414/8414/000000000038",
    "msg": "2015-01-18T18:26:24.834317Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:2) using command: ping6 -c 1 -W 2 abcd::40:40:2 sequence:38"
  },
  {
    "msg-id": "001421605585.003376/rwopenstack-grunt23-vm32/8112/8112/000000000384",
    "msg": "2015-01-18T18:26:25.003376Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:384"
  },
  {
    "msg-id": "001421605585.004252/rwopenstack-grunt23-vm32/8112/8112/000000000389",
    "msg": "2015-01-18T18:26:25.004252Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:389"
  },
  {
    "msg-id": "001421605586.008549/rwopenstack-grunt23-vm32/8414/8414/000000000039",
    "msg": "2015-01-18T18:26:26.008549Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (1.1.1.1) using command: ping -c 1 -W 2 1.1.1.1 sequence:39"
  },
  {
    "msg-id": "001421605586.072547/rwopenstack-grunt23-vm32/8112/8112/000000000397",
    "msg": "2015-01-18T18:26:26.072547Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:397"
  },
  {
    "msg-id": "001421605586.073525/rwopenstack-grunt23-vm32/8112/8112/000000000402",
    "msg": "2015-01-18T18:26:26.073525Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:402"
  },
  {
    "msg-id": "001421605586.076590/rwopenstack-grunt23-vm32/8414/8414/000000000040",
    "msg": "2015-01-18T18:26:26.076590Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (2.2.2.2) using command: ping -c 1 -W 2 2.2.2.2 sequence:40"
  },
  {
    "msg-id": "001421605586.129373/rwopenstack-grunt23-vm32/8112/8112/000000000410",
    "msg": "2015-01-18T18:26:26.129373Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:410"
  },
  {
    "msg-id": "001421605586.129603/rwopenstack-grunt23-vm32/8112/8112/000000000415",
    "msg": "2015-01-18T18:26:26.129603Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:415"
  },
  {
    "msg-id": "001421605586.132506/rwopenstack-grunt23-vm32/8414/8414/000000000041",
    "msg": "2015-01-18T18:26:26.132506Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:1) using command: ping6 -c 1 -W 2 abcd::40:40:1 sequence:41"
  },
  {
    "msg-id": "001421605586.189404/rwopenstack-grunt23-vm32/8112/8112/000000000423",
    "msg": "2015-01-18T18:26:26.189404Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:423"
  },
  {
    "msg-id": "001421605586.190305/rwopenstack-grunt23-vm32/8112/8112/000000000428",
    "msg": "2015-01-18T18:26:26.190305Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:428"
  },
  {
    "msg-id": "001421605586.192551/rwopenstack-grunt23-vm32/8414/8414/000000000042",
    "msg": "2015-01-18T18:26:26.192551Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:2) using command: ping6 -c 1 -W 2 abcd::40:40:2 sequence:42"
  },
  {
    "msg-id": "001421605586.245348/rwopenstack-grunt23-vm32/8112/8112/000000000436",
    "msg": "2015-01-18T18:26:26.245348Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:436"
  },
  {
    "msg-id": "001421605586.245601/rwopenstack-grunt23-vm32/8112/8112/000000000441",
    "msg": "2015-01-18T18:26:26.245601Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:441"
  },
  {
    "msg-id": "001421605587.249338/rwopenstack-grunt23-vm32/8414/8414/000000000043",
    "msg": "2015-01-18T18:26:27.249338Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (1.1.1.1) using command: ping -c 1 -W 2 1.1.1.1 sequence:43"
  },
  {
    "msg-id": "001421605587.378615/rwopenstack-grunt23-vm32/8112/8112/000000000449",
    "msg": "2015-01-18T18:26:27.378615Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:449"
  },
  {
    "msg-id": "001421605587.379445/rwopenstack-grunt23-vm32/8112/8112/000000000454",
    "msg": "2015-01-18T18:26:27.379445Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:454"
  },
  {
    "msg-id": "001421605587.383429/rwopenstack-grunt23-vm32/8414/8414/000000000044",
    "msg": "2015-01-18T18:26:27.383429Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (2.2.2.2) using command: ping -c 1 -W 2 2.2.2.2 sequence:44"
  },
  {
    "msg-id": "001421605587.500339/rwopenstack-grunt23-vm32/8112/8112/000000000462",
    "msg": "2015-01-18T18:26:27.500339Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:462"
  },
  {
    "msg-id": "001421605587.501367/rwopenstack-grunt23-vm32/8112/8112/000000000467",
    "msg": "2015-01-18T18:26:27.501367Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:467"
  },
  {
    "msg-id": "001421605587.505546/rwopenstack-grunt23-vm32/8414/8414/000000000045",
    "msg": "2015-01-18T18:26:27.505546Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:1) using command: ping6 -c 1 -W 2 abcd::40:40:1 sequence:45"
  },
  {
    "msg-id": "001421605587.616367/rwopenstack-grunt23-vm32/8112/8112/000000000475",
    "msg": "2015-01-18T18:26:27.616367Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:475"
  },
  {
    "msg-id": "001421605587.617324/rwopenstack-grunt23-vm32/8112/8112/000000000480",
    "msg": "2015-01-18T18:26:27.617324Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:480"
  },
  {
    "msg-id": "001421605587.620611/rwopenstack-grunt23-vm32/8414/8414/000000000046",
    "msg": "2015-01-18T18:26:27.620611Z rwopenstack-grunt23-vm32::debug[session.py-8414-8414] version:1.0 session.py:228 notification:rw-logger-log.rwlogger-log-debug event-id:18000 groupcallid:1 log:Pinging dns entry (abcd::40:40:2) using command: ping6 -c 1 -W 2 abcd::40:40:2 sequence:46"
  },
  {
    "msg-id": "001421605587.674475/rwopenstack-grunt23-vm32/8112/8112/000000000488",
    "msg": "2015-01-18T18:26:27.674475Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 packet_direction:OUTBOUND sequence:488"
  },
  {
    "msg-id": "001421605587.675400/rwopenstack-grunt23-vm32/8112/8112/000000000493",
    "msg": "2015-01-18T18:26:27.675400Z rwopenstack-grunt23-vm32::info[Logging-8112-8112] version:1.0 NwLogMgr.c:347 notification:rw-appmgr-log.nwepc-ue-packet-dump event-id:20109 data:E groupcallid:1 sequence:493"
  }
];
