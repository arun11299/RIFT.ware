angular.module('global',[])
    .factory('globalCharts',function(){
        var g = {};
        g.traffic = {
            active:false
        };
        g.ip_traffic = {
            active:false
        };
        g.diameter = {
            active:false
        };
        g.toggleState = function(chart) {
            if (chart == 'traffic') {
                g.traffic.active ? g.ip_traffic.active = g.diameter.active = false : g.ip_traffic.active = g.diameter.active = true;
            }
            else {
                console.log(g[chart])
                g[chart].active = !g[chart].active;
                if (!g.ip_traffic.active && !g.diameter.active) {
                    g.traffic.active = false;
                }
                console.log(g[chart])
            }
        }
        return g;
    })