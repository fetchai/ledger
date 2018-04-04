
def make_codes(N, TB):
    B = N * TB
    for i in range(N):
        print "auto &neighbour%d = sites_[ site.indices[ %d ] ];" % (i,i)
    print
    variables = [ ] 
    variables2 = [ ]   
    for i in range(N):
        print "energy_type ds%d = site.spin ^ neighbour%d.spin ^ site.sign[ %d ];" % (i,i, i)
        variables.append( "ds%d" % i )
    print
    print "spin_type flip"
    for i in range(N):
        new_vars = []

        print "energy_type b%d" % i,"="
        #    for i in range(N):
    


make_codes(4, 1)

b1 b2 b3 b4 b5 b6

a1 = b1 ^ b2
a2 = b3 ^ b4
a3 = b5 ^ b6
A1 = b1 & b2
A2 = b3 & b4
A3 = b5 & b6

c1 = a1 ^ a2

c3 = c1 ^ a3
C3 = c1 & a3

C1 = a1 & a2

C4 = C1 ^ c3
C
