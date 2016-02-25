
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import d3 from 'd3';
import DashboardCard from '../dashboard_card/dashboard_card.jsx';

export default class TopologyL2Graph extends React.Component {
    constructor(props) {
        super(props);
        this.data = props.data;
        this.selectedID = 0;
        this.nodeCount = 0;
        this.network_coding = {}
    }
    componentDidMount(){
        //console.log('componentDidMount:props:', this.props);
        var weight = 400;
        this.force = d3.layout.force()
            .size([this.props.width, this.props.height])
            .charge(-weight)
            .linkDistance(weight)
            .on("tick", this.tick.bind(this));

        this.drag = this.force.drag()
            .on("dragstart", this.dragstart);

        this.svg = d3.select(React.findDOMNode(this.refs.topologyL2)).append("svg")
            .attr("width", this.props.width)
            .attr("height", this.props.height)
            .classed("topology", true);
    }
    componentWillUnmount() {
        d3.select('svg').remove();
    }
    componentWillReceiveProps(props) {
        //console.log("TopologyL2Graph.componentWillReceiveProps, props=", props);
        if (props.data.links.length > 0) {
            this.svg.selectAll('*').remove();
            this.network_coding = this.create_group_coding(props.data.network_ids.sort());
            this.update(props.data);
        }
    }
    click = (d) => {
        this.props.selectNode(d);
        this.selectedID = d.id;
    }
    create_group_coding(group_ids) {
        var group_coding = {};
        group_ids.forEach(function(element, index, array) {
            group_coding[element] = index+1;
        });
        return group_coding;
    }
    getNetworkCoding(network_id) {
        var group = this.network_coding[network_id];
        if (group != undefined) {
            return group;
        } else {
            return 0;
        }
    }

    update(graph) {
        //console.log('\n------------\nin TopologyL2Graph.update: source=', graph);

        var svg = this.svg;
        var force = this.force;
        var handler = this;
        force
            .nodes(graph.nodes)
            .links(graph.links)
            .start();

        this.link = svg.selectAll(".link")
            .data(graph.links)
            .enter().append("line")
                .attr("class", "link");

        this.gnodes = svg.selectAll('g.gnode')
            .data(graph.nodes)
            .enter()
            .append('g')
            .classed('gnode', true)
            .attr('data-network', function(d) { return d.network; })
            .attr('class', function(d) {
                return d3.select(this).attr('class') + ' node-group-'+ handler.getNetworkCoding(d.network);
            });

        this.node = this.gnodes.append("circle")
            .attr("class", "node")
            .attr("r", this.props.radius)
            .on("dblclick", this.dblclick)
            .call(this.drag);
        var labels = this.gnodes.append("text")
            .text(function(d) { return d.name; });
    }

    tick = () => {
        this.link.attr("x1", function(d) { return d.source.x; })
               .attr("y1", function(d) { return d.source.y; })
               .attr("x2", function(d) { return d.target.x; })
               .attr("y2", function(d) { return d.target.y; });

        this.gnodes.attr("transform", function(d) {
            return 'translate(' + [d.x, d.y] + ')';
        });

    }

    dblclick = (d) => {
        this.d3.select(this).classed("fixed", d.fixed = false);
    }

    dragstart = (d) => {
        d3.select(this).classed("fixed", d.fixed = true);
    }

    render() {
        //console.log('TopologyL2View.TopologyL2Graph.render called');
        return ( <DashboardCard showHeader={true} title="Topology L2 Graph">
                <div ref="topologyL2"></div>
                </DashboardCard>)
    }
}

TopologyL2Graph.defaultProps = {
    width: 700,
    height: 500,
    maxLabel: 150,
    duration: 500,
    radius: 5,
    data: {
        nodes: [],
        links: [],
        network_ids: []
    }
}
