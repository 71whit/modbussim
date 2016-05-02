
#
# 2016 Whit Schonbein
#
# Purpose: plots selected columns from the results 
# of transposing the output of the extract_registers 
# bro script.
#
# University of New Mexico
# Course: cs544
#

from pylab import *
import sys
#from matplotlib import rc, rcParams

if (len(sys.argv) != 2):
   print('Filename required')
   sys.exit(-1)

# make matrix from input file
matrix = genfromtxt(sys.argv[1])

# Slice out required columns.
a = matrix[:,2]
b = matrix[:,4]
c = matrix[:,6]
d = matrix[:,8]
j = matrix[:,20]

# plot each column
plot(a,label='register 0')
plot(b,label='register 1')
plot(c,label='register 2')
plot(d,label='register 3')
plot(j,label='register 9')

legend(loc='upper right', prop={'size':10})
xlabel('time')
ylabel('rpm')

show()
