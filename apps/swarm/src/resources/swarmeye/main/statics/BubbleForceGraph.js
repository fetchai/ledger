class BubbleForceGraph
{
    dragstarted(d) {
        //console.log("DRAG START", d);
        if (!d3.event.active) {
            //console.log("WAKE", d);
            this.simulation.alphaTarget(0.3).restart();
        }
        d.fx = d.x;
        d.fy = d.y;
    };

    dragged(d) {
        d.fx = d3.event.x;
        d.fy = d3.event.y;
        //console.log("DRAG MORE", d, d3.event);
    };

    dragended(d) {
        //console.log("DRAG END");
        if (!d3.event.active) {
            //console.log("SNOOZE", d);
            this.simulation.alphaTarget(0);
        }
        d.fx = null;
        d.fy = null;
    };

    constructor(targetElementId)
    {
        var self = this;

        this.highestGroupSeenSoFar = 0;

        this.targetElement = document.getElementById(targetElementId);
        this.graphData = {
            nodes: [
                // {
                //     id: "a",
                //     group: 1,
                // },
                // {
                //     id: "b",
                //     group: 1,
                // },
                // {
                //     id: "c",
                //     group: 1,
                // },
            ],
            links: [
                // {
                //     source: "a",
                //     target: "b",
                //     value: 1,
                // },
                // {
                //     source: "b",
                //     target: "c",
                //     value: 7,
                // }
            ],
        };

        this.svg = d3.select("#blobs_here");
        this.color = d3.scaleLinear()
            .domain([ 0, 0.165, 0.33, 0.5, 0.665, 0.81, 1.0 ])
            .range(["red", "yellow", "green", "blue", "cyan", "magenta", "red" ]
                  );

        this.width = 1000;
        this.height = 1000;

        this.svg.append("svg:defs").selectAll("marker")
            .data(["end"])      // Different link/path types can be defined here
            .enter().append("svg:marker")    // This section adds in the arrows
            .attr("id", String)
            .attr("viewBox", "0 -5 10 10")
            .attr("refX", 15)
            .attr("refY", -1.5)
            .attr("markerWidth", 6)
            .attr("markerHeight", 6)
            .attr("orient", "auto")
            .append("svg:path")
            .attr("d", "M0,-5L10,0L0,5")
        ;

        this.linksG = this.svg.append("g").attr("class", "links")
        this.nodesG = this.svg.append("g").attr("class", "nodes")

        this.simulation = d3.forceSimulation();

        this.simulation
            .force("charge",d3.forceManyBody().strength(function(d) { return d.charge || -100; }))
            .force("collide",d3.forceCollide())
            .force("center", d3.forceCenter(this.width / 2, this.height / 2))
            .force("link", d3.forceLink()
                   .id(function(d) { return d.id; })
                   .distance(function(d) { return d.distance || 30; })
                   .strength(2.1)
                  )
        ;

        this.sync();

        this.simulation
            .on("tick", function() {
                self.dataLinks
                    .attr("x1", function(d) {
                        return d.source.x;
                    })
                    .attr("x2", function(d) {
                        return d.target.x;
                    })
                    .attr("y1", function(d) {
                        return d.source.y;
                    })
                    .attr("y2", function(d) {
                        return d.target.y;
                    })
                    .attr("d", function(d) {
                        var dx = d.target.x - d.source.x;
                        var dy = d.target.y - d.source.y;
                        var dr = Math.sqrt(dx * dx + dy * dy);
                        return "M" +
                            d.source.x + "," +
                            d.source.y + "A" +
                            dr + "," + dr + " 0 0,1 " +
                            d.target.x + "," +
                            d.target.y;
                    });
                self.dataLinks
                    .attr("stroke-width", function(d) {
                        if (
                            (d.value === 0) ||
                                (d.value === 0.0) ||
                                (d.value === "0"))
                            return 0.0;
                        if ((!d.value) || (d.value<0.0))
                            return 1.0;
                        return Math.sqrt(d.value);
                    });

                self.dataNodes
                    .attr("transform", function(d){return "translate("+d.x+","+d.y+")"})
                    .selectAll("circle")
                    .attr("r", function(d) { return d.value || 5; })
                    .attr("fill", function(d) {
                        return self.colourForGroupAndStatus(d);
                    })
                ;
                self.dataNodes
                    .selectAll("text")
                    .attr("fill", function(d) {return self.getTextColourForGroup(d); })
            })
        ;
    }


    sync()
    {
        var self = this;

        this.simulation.stop();

        this.simulation
            .nodes(this.graphData.nodes)
        ;


        this.dataNodes = this.nodesG
            .selectAll("g")
            .data(this.graphData.nodes, function(d, i) {
                return d.id;
            });

        this.colourForGroupAndStatus = function(d) {
            var col = d3.rgb(self.color(d.group/16.0));
            if (d.opacity) {
                col.opacity = d.opacity;
            }
            if (d.status == "darken") {
                col = d3.hsl(col).darker(1.5).rgb();
            }
            return col.toString();
        }

        this.getTextColourForGroup = function(d) {
            var col = d3.rgb(self.colourForGroupAndStatus(d));

            var r = col.r / 255.0;
            var g = col.g / 255.0;
            var b = col.b / 255.0;

            r *= .299;
            g *= .587;
            b *= .114;

            var t = r+g+b;

            if ( t < 0.6 ) {
                return "white";
            } else {
                return "black";
            }
        }

        this.getClassForNode = function(d) {
            if (d.class)
                return d.class;
            return "";
        }

        var circles = this.dataNodes
            .enter().append("g");

        circles.append("circle")
            .attr("class", function(d) {
                return self.getClassForNode(d);
            })
            .attr("r", 5)
            .attr("data-creationname", function(d) { return d.id; })
            .attr("fill", function(d) {
                return self.colourForGroupAndStatus(d);
            })
            .call(d3.drag()
                  .on("start", self.dragstarted.bind(this))
                  .on("drag", self.dragged.bind(this))
                  .on("end", self.dragended.bind(this)))
        ;

        circles.append("text")
            .text(function(d) { return d.label; })
            .attr("fill", function(d) { return self.getTextColourForGroup(d); })
            .attr("text-anchor", "middle")
            .attr("alignment-baseline", "middle")
        ;

        this.dataLinks = this.linksG
            .selectAll("path")
            .data(this.graphData.links, function(d, i) {
                return self.getIdForLink(d);
            });

        this.dataLinks
            .enter().append("path")
            .attr("class", "link")
            .attr("marker-end", "url(#end)")
        ;

        this.simulation
            .force("link")
            .links(this.graphData.links)
        ;

        this.dataNodes.exit().remove();
        this.dataLinks.exit().remove();

        this.simulation.alphaTarget(0.01).restart();
    }

    hasNode(node) {
        var self = this;
        return self.graphData.nodes.find(function(element) {
            return node.id == element.id;
        });
    }

    hasLink(link) {
        var self = this;
        var linkName = self.getIdForLink(link);
        //console.log("Do we have? ", linkName);
        return self.graphData.links.find(function(element) {
            var x =  self.getIdForLink(element)
            var r = x == linkName;
            //console.log("Trying", x, r);
            return r;
        });
    }

    getIdForLink(d) {
        if (d && d.id) {
            return d.id;
        }
        if (typeof d.source == "string") {
            var s = `${d.source}-${d.target}`;
            //console.log("FROM G DATA:", s);
            return s;
        }
        var s = `${d.source.id}-${d.target.id}`;
        //console.log("FROM LINK DATA:", s);
        return s;
    }

    addData(g)
    {
        var self = this;

        const newNodes = g.nodes.reduce((res, node) => {
            res[node.id] = node;
            return res;
        }, {})
        const newLinks = g.links.reduce((res, link) => {
            res[self.getIdForLink(link)] = link;
            return res;
        }, {})

        var newNodeNames = new Set(Object.keys(newNodes));
        var oldNodeNames = new Set(self.graphData.nodes.map(node => node.id));

        var newLinkNames = new Set(Object.keys(newLinks));
        var oldLinkNames = new Set(self.graphData.links
                                   .map(link => self.getIdForLink(link)));


        var addableNodes = g.nodes.filter(node => !oldNodeNames.has(node.id))
        var addableLinks = g.links
            .filter(link => !oldLinkNames.has(self.getIdForLink(link)))

        //console.log(addableNodes);

        var killNodes = new Set(self.graphData.nodes
            .map(node => node.id)
            .filter(nodeName => !newNodeNames.has(nodeName)));
        var killLinks = new Set(self.graphData.links
            .map(link => self.getIdForLink(link))
            .filter(linkName => !newLinkNames.has(linkName)));

        //console.log("+N", addableNodes);
        //console.log("+L", addableLinks);

        var preservedNodes = self.graphData.nodes
            .filter(node => !killNodes.has(node.id));
        var preservedLinks = self.graphData.links
            .filter(link => !killNodes.has(self.getIdForLink(link)));

        if (killNodes.size || killLinks.size) {

            for(var i = self.graphData.nodes.length - 1; i >= 0; i--) {
                if (killNodes.has(self.graphData.nodes[i].id)) {
                    self.graphData.nodes.splice(i, 1);
                }
            }

            for(var i = self.graphData.links.length - 1; i >= 0; i--) {
                if (killLinks.has(self.getIdForLink(self.graphData.links[i]))) {
                    self.graphData.links.splice(i, 1);
                }
            }

        }

        var preservedNodesByName = self.graphData.nodes
            .reduce((res, node) => { res[node.id] = node; return res; }, {});

        addableNodes.forEach(node => {
            if (node.inherit) {
                var n = preservedNodesByName[node.inherit];
                if (n) {
                    node.x = n.x;
                    node.y = n.y;
                    return;
                }
            }
            node.x = node.ix || ((Math.random() *.4) +.3) * self.width;
            node.y = node.iy || ((Math.random() *.4) +.3) * self.height;
        });

        debugger;
        self.graphData.links.push(...addableLinks);
        self.graphData.nodes.unshift(...addableNodes);

        var updatedNodesByName = self.graphData.nodes
            .reduce((res, node) => { res[node.id] = node; return res; }, {});
        var updatedLinksByName = self.graphData.links
            .reduce((res, link) => {
                res[self.getIdForLink(link)] = link;
                return res; }, {});

        Object.keys(updatedNodesByName).forEach(k => {
            [
                "group", "status", "value", "charge", "darken", "opacity"
            ].forEach(copykey => {
                updatedNodesByName[k][copykey] =
                    (k in newNodes) ? newNodes[k][copykey] : updatedNodesByName[k][copykey];
            });
        });

        Object.keys(updatedLinksByName).forEach(k => {
            [
                "value", "distance"
            ].forEach(copykey => {
                updatedLinksByName[k][copykey] =
                    (k in newLinks) ? newLinks[k][copykey] : updatedLinksByName[k][copykey];
            });
        });

        self.sync();
    }
};



