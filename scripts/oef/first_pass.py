#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import random as rand
from graph_tool.all import *
import sys, os
from gi.repository import Gtk, Gdk, GdkPixbuf, GObject
import numpy.random as random

rand.seed(42)

home = os.path.expanduser('~')
sys.path.insert(0, home + '/repos/bayeux/')
from p2psims import *

# -------------------- set up ----------------------------

n_trans = 500
n_nodes = 100
#cpu_mean = 3
#cpu_std = 3
#a_time = 16.0

p2p = P2PGraph.oef_network(n_nodes, 2)
g = p2p.graph
seen = g.new_vertex_property('bool')
pos = sfdp_layout(g)

# -------------------- parse commands ----------------------------

# If True, the frames will be dumped to disk as images.
offscreen = sys.argv[1] == "offscreen" if len(sys.argv) > 1 else False
max_count = 500
if offscreen and not os.path.exists("./frames"):
    os.mkdir("./frames")


# -------------------- initial window ----------------------------
# This creates a GTK+ window with the initial graph layout
if not offscreen:
    win = GraphWindow(g, pos, geometry=(500, 400),
                      edge_color=[0.6, 0.6, 0.6, 1],
                      vertex_fill_color=seen)
else:
    count = 0
    win = Gtk.OffscreenWindow()
    win.set_default_size(500, 400)
    win.graph = GraphWidget(g, pos,
                            edge_color=[0.6, 0.6, 0.6, 1],
                            vertex_fill_color=seen)
    win.add(win.graph)


# ------------------ function for changing state --------------------
def update_state():

    state_change = False
    for vertex in g.get_vertices():
        if not seen[vertex]:
            seen[vertex] = True
            state_change = True
            break

    # The following will force the re-drawing of the graph, and issue a
    # re-drawing of the GTK window.
    win.graph.regenerate_surface()
    win.graph.queue_draw()

    # if doing an offscreen animation, dump frame to disk
    if offscreen:
        global count
        pixbuf = win.get_pixbuf()
        pixbuf.savev(r'./frames/bkch%06d.png' % count, 'png', [], [])
        if not state_change:
            sys.exit(0)
        count += 1

    # We need to return True so that the main loop will call this function more
    # than once.
    return True


# Bind the function above as an 'idle' callback.
cid = GObject.idle_add(update_state)

# We will give the user the ability to stop the program by closing the window.
win.connect("delete_event", Gtk.main_quit)

# Actually show the window, and start the main loop.
win.show_all()
Gtk.main()
