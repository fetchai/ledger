
def make_codes(N, TB):
    B = N * TB
    for i in range(N):
        print "auto &neighbour%d = sites_[ site.indices[ %d ] ];" % (i, i)
    print
    variables = []
    variables2 = []
    for i in range(N):
        print "energy_type ds%d = site.spin ^ neighbour%d.spin ^ site.sign[ %d ];" % (i, i, i)
        variables.append("ds%d" % i)
    print
    print "spin_type flip = 0;"
    print "spin_type used = 0;"
    print
    for i in range(N):
        new_vars = []

        print "energy_type b%d" % i, "="
        #    for i in range(N):


make_codes(4, 1)
