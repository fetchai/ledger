


class ForceGraph
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
    
    constructor(targetElement, distance)
    {
	var self = this;
        self.distance = distance;

	this.highestGroupSeenSoFar = 0;

	this.targetElement = targetElement;
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
	
	this.svg = d3.select("svg");
	this.color = d3.scaleOrdinal(d3.schemeCategory10);
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
	    .attr("d", "M0,-5L10,0L0,5");
	
	this.linksG = this.svg.append("g").attr("class", "links")
	this.nodesG = this.svg.append("g").attr("class", "nodes")
	
	this.simulation = d3.forceSimulation();

	this.simulation
	    .force("charge", d3.forceManyBody())
	    .force("center", d3.forceCenter(this.width / 2, this.height / 2))
	;
	
	this.simulation
	    .force("link", d3.forceLink().id(function(d) { return d.id; }).distance(self.distance))
	;

	this.sync();
	
 	this.simulation
	    .on("tick", function() {
		self.dataLinks
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
			return Math.sqrt(d.value) || 1.0;
		    });
		
		self.dataNodes
		    .attr("cx", function(d) { return d.x; })
		    .attr("cy", function(d) { return d.y; });
	    }).on("end", function() {
		console.log("STATIC");
	    });
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
	    .selectAll("circle")
	    .data(this.graphData.nodes, function(d, i) {
		return d.id;
	    });

	this.colourForGroupAndStatus = function(gr, st) {

	    while(gr>this.highestGroupSeenSoFar) {
		this.color(this.highestGroupSeenSoFar);
		this.highestGroupSeenSoFar++;
	    }
	    
	    var r = this.color(gr);
	    if (st != "ok") {
		r = d3.hsl(r).brighter(1.5).rgb().toString();
	    }
	    return r;
	}

	this.dataNodes
	    .enter().append("circle")
	    .attr("r", 5)
	    .attr("data-creationname", function(d) { return d.id; })
	    .attr("fill", "#000")
	    .attr("fill", function(d) {
		return self.colourForGroupAndStatus(d.group, d.status);
	    })
	    .call(d3.drag()
		  .on("start", self.dragstarted.bind(this))
		  .on("drag", self.dragged.bind(this))
		  .on("end", self.dragended.bind(this)))
	;

	var foo =  this.dataNodes
	    .attr("fill", function(d) { return self.colourForGroupAndStatus(d.group, d.status); })
	    .attr("data-name", function(d) { return d.id; })

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
	var oldLinkNames = new Set(self.graphData.links.map(link => self.getIdForLink(link)));

	
	var addableNodes = g.nodes.filter(node => !oldNodeNames.has(node.id))
	var addableLinks = g.links.filter(link => !oldLinkNames.has(self.getIdForLink(link)))

        console.log(addableNodes);
        
	var killNodes = new Set(self.graphData.nodes.map(node => node.id).filter(nodeName => !newNodeNames.has(nodeName)));
	var killLinks = new Set(self.graphData.links.map(link => self.getIdForLink(link)).filter(linkName => !newLinkNames.has(linkName)));
	
	//console.log("+N", addableNodes);
	//console.log("+L", addableLinks);

	var preservedNodes = self.graphData.nodes.filter(node => !killNodes.has(node.id));
	var preservedLinks = self.graphData.links.filter(link => !killNodes.has(self.getIdForLink(link)));
	
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

	self.graphData.links.push(...addableLinks);
	self.graphData.nodes.push(...addableNodes);

	//self.graphData.links.distance(link => { return 30; });

	var updatedNodesByName = self.graphData.nodes.reduce((res, node) => { res[node.id] = node; return res; }, {});
	var updatedLinksByName = self.graphData.links.reduce((res, link) => { res[self.getIdForLink(link)] = link; return res; }, {});

	Object.keys(updatedNodesByName).forEach(k => {
	    updatedNodesByName[k].group = (k in newNodes) ? newNodes[k].group : updatedNodesByName[k].group;
	    updatedNodesByName[k].status = (k in newNodes) ? newNodes[k].status : updatedNodesByName[k].status;
	});

	Object.keys(updatedLinksByName).forEach(k => {
	    updatedLinksByName[k].value = (k in newLinks) ? newLinks[k].value : updatedLinksByName[k].value;
	});

	//if (self.graphData.links.length > 1000) {
	//    debugger;
	//}

	self.sync();
    }
};



