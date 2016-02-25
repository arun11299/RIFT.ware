
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */
import React from 'react';
import d3 from 'd3';

import DashboardCard from '../dashboard_card/dashboard_card.jsx';

export default class TopologyTree extends React.Component {
    constructor(props) {
        super(props);
        this.data = props.data;
        this.selectedID = 0;
        this.nodeCount = 0;
    }
    componentDidMount(){
        this.tree = d3.layout.tree()
            .size([this.props.height, this.props.width]);
        this.diagonal = d3.svg.diagonal()
            .projection(function(d) { return [d.y, d.x]; });

        this.svg = d3.select(React.findDOMNode(this.refs.topology)).append("svg")
            .attr("width", this.props.width)
            .attr("height", this.props.height)
            .append("g")
            .attr("transform", "translate(" + this.props.maxLabel + ",0)");
    }
    componentWillReceiveProps(props) {
        if(props.data.hasOwnProperty('type') && !this.props.hasSelected) {
            this.selectedID = props.data.id;
            this.props.selectNode(props.data);
        }
        this.update(props.data);
    }
    computeRadius(d) {
        // if(d.parameters && d.parameters.vcpu) {
        //     return this.props.radius + d.parameters.vcpu.total;
        // } else {
            return this.props.radius;
        // }
    }
    click = (d) => {
        this.props.selectNode(d);
        this.selectedID = d.id;
        // if (d.children){
        //     d._children = d.children;
        //     d.children = null;
        // }
        // else{
        //     d.children = d._children;
        //     d._children = null;
        // }
        // this.update(d);
    }
    update(source) {
        // Compute the new tree layout.
        var svg = this.svg;
        var nodes = this.tree.nodes(source).reverse();
        var links = this.tree.links(nodes);
        var self = this;

        // Normalize for fixed-depth.
        nodes.forEach(function(d) { d.y = d.depth * self.props.maxLabel; });

        // Update the nodes…
        var node = svg.selectAll("g.node")
            .data(nodes, function(d){
                return d.id || (d.id = ++self.nodeCount);
            });

        // Enter any new nodes at the parent's previous position.
        var nodeEnter = node.enter()
            .append("g")
            .attr("class", "node")
            .attr("transform", function(d){ return "translate(" + source.y0 + "," + source.x0 + ")"; })
            .on("click", this.click);

        nodeEnter.append("circle")
            .attr("r", 0)
            .style("fill", function(d){
                return d._children ? "lightsteelblue" : "white";
            });

        nodeEnter.append("text")
            .attr("x", function(d){
                var spacing = self.computeRadius(d) + 5;
                    return d.children || d._children ? -spacing : spacing;
            })
            .attr("transform", function(d, i) { return "translate(0," + ((i%2) ? 15 : -15) + ")"; })
            .attr("dy", "3")
            .attr("text-anchor", function(d){ return d.children || d._children ? "end" : "start"; })
            .text(function(d){ return d.name; })
            .style("fill-opacity", 0);

        // Transition nodes to their new position.
        var nodeUpdate = node
            .transition()
            .duration(this.props.duration)
            .attr("transform", function(d) { return "translate(" + d.y + "," + d.x + ")"; });

        nodeUpdate.select("circle")
            .attr("r", function(d){ return self.computeRadius(d); })
            .style("fill", function(d) { return d.id == self.selectedID ? "green" : "lightsteelblue"; });

        nodeUpdate.select("text")
            .style("fill-opacity", 1)
            .style("font-weight", function(d) {
                return d.id == self.selectedID ? "900" : "inherit";
            })
            .attr("transform", function(d, i) {
                return "translate(0," + ((i%2) ? 15 : -15) + ")" + (d.id == self.selectedID ? "scale(1.125)" : "scale(1)");
            });

        // Transition exiting nodes to the parent's new position.
        var nodeExit = node.exit()
            .transition()
            .duration(this.props.duration)
            .attr("transform", function(d) { return "translate(" + source.y + "," + source.x + ")"; })
            .remove();

        nodeExit.select("circle").attr("r", 0);
        nodeExit.select("text").style("fill-opacity", 0);

        // Update the links…
        var link = svg.selectAll("path.link")
            .data(links, function(d){ return d.target.id; });

        // Enter any new links at the parent's previous position.
        link.enter().insert("path", "g")
            .attr("class", "link")
            .attr("d", function(d){
                var o = {x: source.x0, y: source.y0};
                return self.diagonal({source: o, target: o});
            });

        // Transition links to their new position.
        link
            .transition()
            .duration(this.props.duration)
            .attr("d", self.diagonal);

        // Transition exiting nodes to the parent's new position.
        link.exit()
            .transition()
            .duration(this.props.duration)
            .attr("d", function(d){
                var o = {x: source.x, y: source.y};
                return self.diagonal({source: o, target: o});
            })
            .remove();

        // Stash the old positions for transition.
        nodes.forEach(function(d){
            d.x0 = d.x;
            d.y0 = d.y;
        });
    }
    render() {
        return ( <DashboardCard showHeader={true} title="Topology Tree">
                <div ref="topology"></div>
                </DashboardCard>)
    }
}

TopologyTree.defaultProps = {
    width: 1000,
    height: 500,
    maxLabel: 150,
    duration: 500,
    radius: 5,
    data: {}
}
